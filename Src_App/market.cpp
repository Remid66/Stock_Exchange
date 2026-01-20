#include "market.hpp"


// constructor
Market::Market(Database_Manager& database) : Exchange_Price(0.0), Database(database)
{

}

// implement a move constructor
Market::Market(Market&& other) noexcept : Buy_Orders(std::move(other.Buy_Orders)), Sell_Orders(std::move(other.Sell_Orders)), Exchange_Price(other.Exchange_Price), Database(other.Database)
{

}

// implement move assignment operator
Market& Market::operator=(Market&& other) noexcept
{
    if (this != &other){
        Buy_Orders = std::move(other.Buy_Orders);
        Sell_Orders = std::move(other.Sell_Orders);
        Exchange_Price = other.Exchange_Price;
        // Database reference remains unchanged
    }
    return *this;
}


// getters
Database_Manager& Market::get_database() const
{
    return Database;
}


// clients handling
// deposit funds into the account of a client
void Market::deposit(const ID& client_id, const double& amount)
{
    Client client(client_id, Database);
    client.deposit(amount);
}

// withdraw funds from the account of a client
void Market::withdraw(const ID& client_id, const double& amount)
{
    Client client(client_id, Database);
    client.withdraw(amount);
}

// returns True if the amount can be withdrawn from the client balance
bool Market::can_afford(const ID& client_id, const int& quantity, const double& price, const ID& action_id) const
{
    Client client(client_id, Database);
    return client.can_afford(quantity, price, action_id);
}

// returns True if the action can be removed from the portfolio of the client
bool Market::has_shares(const ID& client_id, const ID& action_id, const int& quantity) const
{
    Client client(client_id, Database);
    return client.has_shares(action_id, quantity);
}

// check if a client exists
bool Market::client_exists(const ID& client_id) const
{
    std::string query = fmt::format(
        "SELECT client_id FROM clients WHERE client_id = {}",
        client_id
    );
    return Database.execute_SQL_query_ID(query) == client_id;
}

// check if a client exists with the given name
bool Market::client_name_exists(const std::string& client_name) const
{
    std::string query = fmt::format(
        "SELECT client_id FROM clients WHERE name = '{}'",
        client_name
    );
    return Database.execute_SQL_query_ID(query) != -1;
}

// check if a client is registered with the given name and password and return its ID
ID Market::client_id_if_name_and_password_registered(const std::string& client_name, const std::string& client_password)
{
    ID client_id = -1;  // default value in case no client is found
    // convert the client password to the appropriate encrypted format (binary)
    std::vector<unsigned char> encrypted_password(client_password.begin(), client_password.end());

    // prepare the SQL query to search for the client by name and encrypted password
    std::string query = "SELECT client_id FROM clients WHERE name = ? AND encrypted_password = ?";

    sqlite3_stmt* stmt;
    // prepare the SQL query
    if (sqlite3_prepare_v2(Database.get_database(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(Database.get_database()) << std::endl;
        return client_id;
    }

    // bind the client_name to the first placeholder
    sqlite3_bind_text(stmt, 1, client_name.c_str(), -1, SQLITE_STATIC);
    // bind the encrypted_password (as BLOB) to the second placeholder
    sqlite3_bind_blob(stmt, 2, encrypted_password.data(), encrypted_password.size(), SQLITE_STATIC);

    // execute the query
    if (sqlite3_step(stmt) == SQLITE_ROW){
        // fetch the client ID from the result
        client_id = sqlite3_column_int(stmt, 0); // assuming the first column is the client_id
    }
    // finalize the statement
    sqlite3_finalize(stmt);
    return client_id;
}

// add a client to the market with its balance and portfolio (action_id and quantity)
void Market::add_client(const ID& client_id, const std::string& client_name, const std::string& client_password, const double& balance, std::unordered_map<ID, int> portfolio)
{
    // convert the std::string password to std::vector<unsigned char> for storage as BLOB
    std::vector<unsigned char> encrypted_password(client_password.begin(), client_password.end());
    
    // insert client into the "clients" table
    std::string query = "INSERT INTO clients (client_id, name, encrypted_password, balance) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(Database.get_database(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK){
        // bind client_id (INTEGER)
        sqlite3_bind_int(stmt, 1, client_id);
        // bind client_name (TEXT)
        sqlite3_bind_text(stmt, 2, client_name.c_str(), -1, SQLITE_STATIC);
        // bind encrypted_password (BLOB)
        sqlite3_bind_blob(stmt, 3, encrypted_password.data(), encrypted_password.size(), SQLITE_STATIC);
        // bind balance (REAL)
        sqlite3_bind_double(stmt, 4, balance);
        // execute the insert statement
        if (sqlite3_step(stmt) != SQLITE_DONE){
            std::cerr << "Error inserting client into database: " << sqlite3_errmsg(Database.get_database()) << std::endl;
        }
        sqlite3_finalize(stmt);  // finalize the statement after execution
    }
    else {
        std::cerr << "Error preparing SQL insert statement for client: " << sqlite3_errmsg(Database.get_database()) << std::endl;
    }
    
    for (const auto& [action_id, quantity] : portfolio){
        // the action will not already be in the client's portfolio since we are creating the client
        std::string query = fmt::format(
            "INSERT INTO client_portfolio (client_id, action_id, quantity) VALUES ({}, {}, {})",
            client_id,
            action_id,
            quantity
        );
        Database.execute_SQL(query);
    }
}

// remove a client from the market
void Market::remove_client(const ID& client_id)
{   
    std::string query = fmt::format(
        "DELETE FROM clients WHERE client_id = {}",
        client_id
    );
    Database.execute_SQL(query);
    query = fmt::format(
        "DELETE FROM client_portfolio WHERE client_id = {}",
        client_id
    );
    Database.execute_SQL(query);
    query = fmt::format(
        "DELETE FROM orders WHERE client_id = {}",
        client_id
    );
    Database.execute_SQL(query);
}

// get the client id from a client name
ID Market::get_client_id_from_name(const std::string& client_name) const 
{
    std::string query = fmt::format(
        "SELECT client_id FROM clients WHERE name = '{}'",
        client_name
    );
    return Database.execute_SQL_query_ID(query);
}

// update the portfolio of a client with a new action
void Market::update_client_portfolio(const ID& client_id, const Order_Type& order_type, const ID& action_id, const int& quantity, const double& price, const ID& daily_time, const ID& date_time)
{
    Client client(client_id, Database);
    client.update_portfolio(order_type, action_id, quantity, price, daily_time, date_time);
}

// add an order to the completed orders of a client
void Market::add_order_to_client_completed_orders(const ID& client_id, const ID& order_id, const ID& order_time_date, const ID& order_time_daily, const Order_Type& order_type, const int& quantity, const ID& action_id, const Order_Trigger& trigger_type, const double& price, const double& trigger_price_lower, const double& trigger_price_upper, const ID& expiration_time_date, const ID& expiration_time_daily)
{
    Client client(client_id, Database);
    client.add_completed_order(order_id, order_time_date, order_time_daily, order_type, quantity, action_id, trigger_type, price, trigger_price_lower, trigger_price_upper, expiration_time_date, expiration_time_daily);
}

// add an order to the pending orders of a client
void Market::add_order_to_client_pending_orders(const ID& client_id, const ID& order_id, const ID& order_time_date, const ID& order_time_daily, const Order_Type& order_type, const int& quantity, const ID& action_id, const Order_Trigger& trigger_type, const double& price, const double& trigger_price_lower, const double& trigger_price_upper, const ID& expiration_time_date, const ID& expiration_time_daily)
{
    Client client(client_id, Database);
    client.add_pending_order(order_id, order_time_date, order_time_daily, order_type, quantity, action_id, trigger_type, price, trigger_price_lower, trigger_price_upper, expiration_time_date, expiration_time_daily);
}

// remove an order from the pending orders of a client
void Market::remove_order_from_client_pending_orders(const ID& client_id, const ID& order_id)
{
    std::string query = fmt::format(
        "DELETE FROM orders WHERE order_id = {} AND order_status = 'PENDING' AND client_id = {}",
        order_id,
        client_id
    );
    Database.execute_SQL(query);
}


// actions handling
// check if an action exists
bool Market::action_exists(const ID& action_id) const
{
    std::string query = fmt::format(
        "SELECT action_id FROM actions WHERE action_id = {}",
        action_id
    );
    return Database.execute_SQL_query_ID(query) == action_id;
}

// add an action to the market
void Market::add_action(const ID& action_id, const std::string& name, const int& quantity, const double& price, const ID& daily_time, const ID& date_time)
{
    // if the action is already in the market, we add the quantity
    if (action_exists(action_id)){
        std::string query = fmt::format(
            "UPDATE actions SET quantity = quantity + {} WHERE action_id = {}",
            quantity,
            action_id
        );
        Database.execute_SQL(query);
    }
    // otherwise, we add the action to the market
    else {
        std::string query = fmt::format(
            "INSERT INTO actions (action_id, name, quantity) VALUES ({}, '{}', {})",
            action_id,
            name,
            quantity
        );
        Database.execute_SQL(query);
    }
    std::string query2 = fmt::format(
        "INSERT INTO prices (action_id, price, date_time, daily_time) VALUES ({}, {}, {}, {})",
        action_id,
        price,
        daily_time,
        date_time
    );
    Database.execute_SQL(query2);
}

// remove an action from the market
void Market::remove_action(const ID& action_id)
{
    std::string query = fmt::format(
        "DELETE FROM actions WHERE action_id = {}",
        action_id
    );
    Database.execute_SQL(query);
    query = fmt::format(
        "DELETE FROM prices WHERE action_id = {}",
        action_id
    );
    Database.execute_SQL(query);
    query = fmt::format(
        "DELETE FROM orders WHERE action_id = {}",
        action_id
    );
    Database.execute_SQL(query);
    query = fmt::format(
        "DELETE FROM client_portfolio WHERE action_id = {}",
        action_id
    );
    Database.execute_SQL(query);
}

// get the market value (sum of the values of all the actions)
double Market::get_market_value() const
{
    std::string query = R"(
        SELECT SUM(a.quantity * p.price) 
        FROM actions a 
        LEFT JOIN prices p ON a.action_id = p.action_id
        WHERE (p.date_time, p.daily_time) = (
            SELECT p2.date_time, p2.daily_time
            FROM prices p2
            WHERE p2.action_id = a.action_id
            AND p2.date_time = (
                SELECT MAX(date_time)
                FROM prices
                WHERE action_id = p2.action_id
            )
            AND p2.daily_time = (
                SELECT MAX(daily_time)
                FROM prices
                WHERE action_id = p2.action_id
                AND date_time = p2.date_time
            )
        )
    )";
    return Database.execute_SQL_query_double(query);
}


// market functionment
// accumulate an order to the market and sort the orders by priority (add the order to the pending orders for the client)
void Market::accumulate_order(const ID& client_id, const ID& order_id, const ID& order_time_date, const ID& order_time_daily, const Order_Type& order_type, const int& quantity, const ID& action_id, const Order_Trigger& trigger_type, const double& price, const double& trigger_price_lower, const double& trigger_price_upper, const ID& expiration_time_date, const ID& expiration_time_daily)
{
    // check if the client exists
    if (!client_exists(client_id)){
        std::cerr << "Error: Client with ID " << client_id << " not found.\n";
        return;
    }

    // add the order to the pending orders of the client
    add_order_to_client_pending_orders(client_id, order_id, order_time_date, order_time_daily, order_type, quantity, action_id, trigger_type, price, trigger_price_lower, trigger_price_upper, expiration_time_date, expiration_time_daily);

    // create a shared pointer for the new order
    auto order = std::make_unique<Order>(order_id, Database);

    // accumulate the order to the market and sort the orders by priority
    if (order_type == Order_Type::BUY){
        auto buy_cmp = [](const std::unique_ptr<Order>& a, const std::unique_ptr<Order>& b){
            if (a->get_price() != b->get_price())
                return a->get_price() > b->get_price(); // higher price first
            if (a->get_date_order_time() != b->get_date_order_time())
                return a->get_date_order_time() < b->get_date_order_time(); // earlier date first
            return a->get_daily_order_time() < b->get_daily_order_time(); // earlier intraday time
        };
        auto& orders = Buy_Orders[action_id];
        auto it = std::lower_bound(orders.begin(), orders.end(), order, buy_cmp);
        orders.insert(it, std::move(order));
    }
    else if (order_type == Order_Type::SELL){
        auto sell_cmp = [](const std::unique_ptr<Order>& a, const std::unique_ptr<Order>& b){
            if (a->get_price() != b->get_price())
                return a->get_price() < b->get_price(); // lower price first
            if (a->get_date_order_time() != b->get_date_order_time())
                return a->get_date_order_time() < b->get_date_order_time(); // earlier date first
            return a->get_daily_order_time() < b->get_daily_order_time(); // earlier intraday time
        };
        auto& orders = Sell_Orders[action_id];
        auto it = std::lower_bound(orders.begin(), orders.end(), order, sell_cmp);
        orders.insert(it, std::move(order));
    }
}

// remove an order from the pending orders of the client (if it exists) and remove it from the market orders by making again the market sorting
void Market::deaccumulate_order(const ID& client_id, const ID& order_id, const Order_Type& order_type, const ID& action_id)
{
    // check if the client exists
    if (!client_exists(client_id)){
        std::cerr << "Error: Client with ID " << client_id << " not found.\n";
        return;
    }

    // remove the order from the pending orders of the client
    remove_order_from_client_pending_orders(client_id, order_id);

    // create a shared pointer for the order to remove
    auto order = std::make_unique<Order>(order_id, Database);

     // Select the appropriate order list
    auto& orders = (order_type == Order_Type::BUY) ? Buy_Orders[action_id] : Sell_Orders[action_id];
    
    // find the matching order and erase it
    auto it = std::find_if(orders.begin(), orders.end(),
        [&](const std::unique_ptr<Order>& o){
            return o->get_order_id() == order_id;
        });

    if (it != orders.end()){
        orders.erase(it);
    } 
    else {
        std::cerr << "Warning: Order with ID " << order_id << " not found in market orders.\n";
    }

}

// process the fixing of the price to order the transactions by priority and update the client's portfolio
void Market::process_fixing()
{
    bool is_new_order_added = false;
    
    // create copies by cloning the elements inside the orders
    std::unordered_map<ID, std::vector<std::unique_ptr<Order>>> buy_orders_copy;
    for (auto& [action_id, orders] : Buy_Orders){
        std::vector<std::unique_ptr<Order>> cloned_orders;
        for (const auto& order : orders){
            cloned_orders.push_back(order->clone());
        }
        buy_orders_copy[action_id] = std::move(cloned_orders);
    }
    std::unordered_map<ID, std::vector<std::unique_ptr<Order>>> sell_orders_copy;
    for (auto& [action_id, orders] : Sell_Orders){
        std::vector<std::unique_ptr<Order>> cloned_orders;
        for (const auto& order : orders){
            cloned_orders.push_back(order->clone());
        }
        sell_orders_copy[action_id] = std::move(cloned_orders);
    }

    for (auto& [action_id, orders] : buy_orders_copy){
        auto& sell_orders_for_action = sell_orders_copy[action_id]; // get the sell orders associated with the action

        // sort buy orders (high price first, then time if prices are equal) and sell orders (low price first, then time if prices are equal)
        std::sort(orders.begin(), orders.end(), 
            [](const std::unique_ptr<Order>& a, const std::unique_ptr<Order>& b){
                return (a->get_price() > b->get_price()) 
                    || (a->get_price() == b->get_price() && a->get_date_order_time() < b->get_date_order_time())
                    || (a->get_price() == b->get_price() && a->get_date_order_time() == b->get_date_order_time() && a->get_daily_order_time() < b->get_daily_order_time());});
        std::sort(sell_orders_for_action.begin(), sell_orders_for_action.end(), 
            [](const std::unique_ptr<Order>& a, const std::unique_ptr<Order>& b){
                return (a->get_price() < b->get_price()) 
                    || (a->get_price() == b->get_price() && a->get_date_order_time() < b->get_date_order_time())
                    || (a->get_price() == b->get_price() && a->get_date_order_time() == b->get_date_order_time() && a->get_daily_order_time() < b->get_daily_order_time());});

        int buy_index = 0, sell_index = 0;
        while (buy_index < orders.size() && sell_index < sell_orders_for_action.size()){
            
            // getting some useful information
            auto& buy_order = orders[buy_index];
            auto& sell_order = sell_orders_for_action[sell_index];

            // buyer 
            std::string buyer_order_info = buy_order->get_order_info();
            std::istringstream buyer_stream(buyer_order_info);
            ID buyer_order_id, buyer_order_time_date, buyer_order_time_daily, buyer_client_id, buyer_expiration_time_date, buyer_expiration_time_daily;
            std::string buyer_trigger_type_str;
            int buyer_quantity;
            double buyer_price, buyer_trigger_price_lower, buyer_trigger_price_upper;
            // extract the values
            buyer_stream >> buyer_order_id 
                            >> buyer_order_time_date 
                            >> buyer_order_time_daily 
                            >> buyer_client_id 
                            >> buyer_quantity 
                            >> buyer_trigger_type_str 
                            >> buyer_price 
                            >> buyer_trigger_price_lower 
                            >> buyer_trigger_price_upper 
                            >> buyer_expiration_time_date 
                            >> buyer_expiration_time_daily;
            Order_Trigger buyer_trigger_type = string_to_trigger(buyer_trigger_type_str);

            // seller
            std::string seller_order_info = sell_order->get_order_info();
            std::istringstream seller_stream(seller_order_info);
            ID seller_order_id, seller_order_time_date, seller_order_time_daily, seller_client_id, seller_expiration_time_date, seller_expiration_time_daily;
            std::string seller_trigger_type_str;
            int seller_quantity;
            double seller_price, seller_trigger_price_lower, seller_trigger_price_upper;
            // extract the values
            seller_stream >> seller_order_id 
                            >> seller_order_time_date 
                            >> seller_order_time_daily 
                            >> seller_client_id 
                            >> seller_quantity 
                            >> seller_trigger_type_str 
                            >> seller_price 
                            >> seller_trigger_price_lower 
                            >> seller_trigger_price_upper 
                            >> seller_expiration_time_date 
                            >> seller_expiration_time_daily;
            Order_Trigger seller_trigger_type = string_to_trigger(seller_trigger_type_str);

            // check if clients exist
            if (!client_exists(buyer_client_id) || !client_exists(seller_client_id)){
                std::cerr << "Error: One of the clients does not exist in the market.\n";
                break;
            }

            // check if the price conditions are met
            if (buyer_price < seller_price){
                break; // no possible transaction for this action
            }

            //  the quantities must be positive
            if (buyer_quantity <= 0 || seller_quantity <= 0){
                break; // no possible transaction for this action
            }

            // perform transaction between buyer and seller
            int transaction_quantity = std::min(buyer_quantity, seller_quantity);
            Exchange_Price = seller_price;
            Time exchange_time = get_current_time_ms();
            ID exchange_time_daily = get_daily_time(exchange_time);
            ID exchange_time_date = get_date_time(exchange_time);

            // update the client's portfolio
            update_client_portfolio(buyer_client_id, Order_Type::BUY, action_id, transaction_quantity, Exchange_Price, exchange_time_daily, exchange_time_date);
            update_client_portfolio(seller_client_id, Order_Type::SELL, action_id, transaction_quantity, Exchange_Price, exchange_time_daily, exchange_time_date);

            // log transaction details
            std::string transaction_details = fmt::format(
                "Transaction of {} actions {} at the price of {}$ between buyer {} and seller {} at time {}",
                transaction_quantity, 
                action_id, 
                Exchange_Price, 
                buyer_client_id, 
                seller_client_id, 
                time_to_string(exchange_time)
            );
            Message transaction_message(get_database().get_new_message_id(), get_database());
            transaction_message.log_message(
                0, 
                Message::Sender::SERVER_MESSAGE, 
                Message::Type::TRANSACTION, 
                transaction_details,
                exchange_time
            );

            // if an order is completely filled, remove it from pending orders
            remove_order_from_client_pending_orders(buyer_client_id, buyer_order_id);
            remove_order_from_client_pending_orders(seller_client_id, seller_order_id);
            
            // add executed portion to completed orders
            if (transaction_quantity > 0){
                add_order_to_client_completed_orders(buyer_client_id, get_database().get_new_order_id(), exchange_time_date, exchange_time_daily, Order_Type::BUY, transaction_quantity, action_id, buyer_trigger_type, Exchange_Price, buyer_trigger_price_lower, buyer_trigger_price_upper, buyer_expiration_time_date, buyer_expiration_time_daily);
                add_order_to_client_completed_orders(seller_client_id, get_database().get_new_order_id(), exchange_time_date, exchange_time_daily, Order_Type::SELL, transaction_quantity, action_id, seller_trigger_type, Exchange_Price, seller_trigger_price_lower, seller_trigger_price_upper, seller_expiration_time_date, seller_expiration_time_daily);
            }
            // if there is remaining quantity, remove the old order and add an updated one
            if (buyer_quantity > transaction_quantity){
                ID new_buyer_order_id = get_database().get_new_order_id();
                add_order_to_client_pending_orders(buyer_client_id, new_buyer_order_id, buyer_order_time_date, buyer_order_time_daily, Order_Type::BUY, buyer_quantity - transaction_quantity, action_id, buyer_trigger_type, buyer_price, buyer_trigger_price_lower, buyer_trigger_price_upper, buyer_expiration_time_date, buyer_expiration_time_daily);
                Buy_Orders[action_id].push_back(std::make_unique<Order>(new_buyer_order_id, Database));
                is_new_order_added = true;
            }
            if (seller_quantity > transaction_quantity){
                ID new_seller_order_id = get_database().get_new_order_id();
                add_order_to_client_pending_orders(seller_client_id, new_seller_order_id, seller_order_time_date, seller_order_time_daily, Order_Type::SELL, seller_quantity - transaction_quantity, action_id, seller_trigger_type, seller_price, seller_trigger_price_lower, seller_trigger_price_upper, seller_expiration_time_date, seller_expiration_time_daily);
                Sell_Orders[action_id].push_back(std::make_unique<Order>(new_seller_order_id, Database));
                is_new_order_added = true;
            }

            // update order quantities before checking completion
            buy_order->set_quantity(buyer_quantity - transaction_quantity);
            sell_order->set_quantity(seller_quantity - transaction_quantity);

            // if buy order is fully executed, move to the next one 
            if (buy_order->get_quantity() == 0){
                buy_index++;
            }
            // if sell order is fully executed, move to the next one
            if (sell_order->get_quantity() == 0){
                sell_index++;
            }
        }

        // remove completed orders
        orders.erase(std::remove_if(orders.begin(), orders.end(), [](const std::unique_ptr<Order>& order){return order->get_quantity() == 0;}), orders.end());
        sell_orders_for_action.erase(std::remove_if(sell_orders_for_action.begin(), sell_orders_for_action.end(), [](const std::unique_ptr<Order>& order){return order->get_quantity() == 0;}), sell_orders_for_action.end());
    }

    // if new orders were added, we call the function again to process them
    if (is_new_order_added){
        process_fixing();
    }
}

// process the continuous trading of the market, transactions between buyers and sellers of different actions
void Market::process_continuous_trading()
{
    bool is_new_order_added = false;
    
    // create copies by cloning the elements inside the orders
    std::unordered_map<ID, std::vector<std::unique_ptr<Order>>> buy_orders_copy;
    for (auto& [action_id, orders] : Buy_Orders){
        std::vector<std::unique_ptr<Order>> cloned_orders;
        for (const auto& order : orders){
            cloned_orders.push_back(order->clone());
        }
        buy_orders_copy[action_id] = std::move(cloned_orders);
    }
    std::unordered_map<ID, std::vector<std::unique_ptr<Order>>> sell_orders_copy;
    for (auto& [action_id, orders] : Sell_Orders){
        std::vector<std::unique_ptr<Order>> cloned_orders;
        for (const auto& order : orders){
            cloned_orders.push_back(order->clone());
        }
        sell_orders_copy[action_id] = std::move(cloned_orders);
    }

    for (auto& [action_id, orders] : buy_orders_copy){
        auto& sell_orders_for_action = sell_orders_copy[action_id]; // get the sell order associated with the action

        auto buy_it = orders.begin();
        while (buy_it != orders.end()){
            auto& buy_order = *buy_it;
            auto sell_it = sell_orders_for_action.begin();
            while (sell_it != sell_orders_for_action.end()){
                auto& sell_order = *sell_it;

                // check if the price conditions are met and orders have a positive quantity
                if (buy_order->get_price() >= sell_order->get_price() && buy_order->get_quantity() > 0 && sell_order->get_quantity() > 0){

                    // buyer 
                    std::string buyer_order_info = buy_order->get_order_info();
                    std::istringstream buyer_stream(buyer_order_info);
                    ID buyer_order_id, buyer_order_time_date, buyer_order_time_daily, buyer_client_id, buyer_expiration_time_date, buyer_expiration_time_daily;
                    std::string buyer_trigger_type_str;
                    int buyer_quantity;
                    double buyer_price, buyer_trigger_price_lower, buyer_trigger_price_upper;
                    // extract the values
                    buyer_stream >> buyer_order_id 
                                    >> buyer_order_time_date 
                                    >> buyer_order_time_daily 
                                    >> buyer_client_id 
                                    >> buyer_quantity 
                                    >> buyer_trigger_type_str 
                                    >> buyer_price 
                                    >> buyer_trigger_price_lower 
                                    >> buyer_trigger_price_upper 
                                    >> buyer_expiration_time_date 
                                    >> buyer_expiration_time_daily;
                    Order_Trigger buyer_trigger_type = string_to_trigger(buyer_trigger_type_str);

                    // seller
                    std::string seller_order_info = sell_order->get_order_info();
                    std::istringstream seller_stream(seller_order_info);
                    ID seller_order_id, seller_order_time_date, seller_order_time_daily, seller_client_id, seller_expiration_time_date, seller_expiration_time_daily;
                    std::string seller_trigger_type_str;
                    int seller_quantity;
                    double seller_price, seller_trigger_price_lower, seller_trigger_price_upper;
                    // extract the values
                    seller_stream >> seller_order_id 
                                    >> seller_order_time_date 
                                    >> seller_order_time_daily 
                                    >> seller_client_id 
                                    >> seller_quantity 
                                    >> seller_trigger_type_str 
                                    >> seller_price 
                                    >> seller_trigger_price_lower 
                                    >> seller_trigger_price_upper 
                                    >> seller_expiration_time_date 
                                    >> seller_expiration_time_daily;
                    Order_Trigger seller_trigger_type = string_to_trigger(seller_trigger_type_str);

                    // check if clients exist
                    if (!client_exists(buyer_client_id) || !client_exists(seller_client_id)){
                        std::cerr << "Error: One of the clients does not exist in the market.\n";
                        continue;
                    }

                    // perform transaction between buyer and seller
                    int transaction_quantity = std::min(buyer_quantity, seller_quantity);
                    Exchange_Price = seller_price;
                    Time exchange_time = get_current_time_ms();
                    ID exchange_time_daily = get_daily_time(exchange_time);
                    ID exchange_time_date = get_date_time(exchange_time);
        
                    // update the client's portfolio
                    update_client_portfolio(buyer_client_id, Order_Type::BUY, action_id, transaction_quantity, Exchange_Price, exchange_time_daily, exchange_time_date);
                    update_client_portfolio(seller_client_id, Order_Type::SELL, action_id, transaction_quantity, Exchange_Price, exchange_time_daily, exchange_time_date);
        
                    // log transaction details
                    std::string transaction_details = fmt::format(
                        "Transaction of {} actions {} at the price of {}$ between buyer {} and seller {} at time {}",
                        transaction_quantity, 
                        action_id, 
                        Exchange_Price, 
                        buyer_client_id, 
                        seller_client_id, 
                        time_to_string(exchange_time)
                    );
                    Message transaction_message(get_database().get_new_message_id(), get_database());
                    transaction_message.log_message(
                        0, 
                        Message::Sender::SERVER_MESSAGE, 
                        Message::Type::TRANSACTION, 
                        transaction_details,
                        exchange_time
                    );
        
                    // if an order is completely filled, remove it from pending orders
                    remove_order_from_client_pending_orders(buyer_client_id, buyer_order_id);
                    remove_order_from_client_pending_orders(seller_client_id, seller_order_id);
                    
                    // add executed portion to completed orders
                    if (transaction_quantity > 0){
                        add_order_to_client_completed_orders(buyer_client_id, get_database().get_new_order_id(), exchange_time_date, exchange_time_daily, Order_Type::BUY, transaction_quantity, action_id, buyer_trigger_type, Exchange_Price, buyer_trigger_price_lower, buyer_trigger_price_upper, buyer_expiration_time_date, buyer_expiration_time_daily);
                        add_order_to_client_completed_orders(seller_client_id, get_database().get_new_order_id(), exchange_time_date, exchange_time_daily, Order_Type::SELL, transaction_quantity, action_id, seller_trigger_type, Exchange_Price, seller_trigger_price_lower, seller_trigger_price_upper, seller_expiration_time_date, seller_expiration_time_daily);
                    }

                    // if there is remaining quantity, remove the old order and add an updated one
                    if (buyer_quantity > transaction_quantity){
                        ID new_buyer_order_id = get_database().get_new_order_id();
                        add_order_to_client_pending_orders(buyer_client_id, new_buyer_order_id, buyer_order_time_date, buyer_order_time_daily, Order_Type::BUY, buyer_quantity-transaction_quantity, action_id, buyer_trigger_type, buyer_price, buyer_trigger_price_lower, buyer_trigger_price_upper, buyer_expiration_time_date, buyer_expiration_time_daily);
                        Buy_Orders[action_id].push_back(std::make_unique<Order>(new_buyer_order_id, Database));
                        is_new_order_added = true;
                    }
                    if (seller_quantity > transaction_quantity){
                        ID new_seller_order_id = get_database().get_new_order_id();
                        add_order_to_client_pending_orders(seller_client_id, new_seller_order_id, seller_order_time_date, seller_order_time_daily, Order_Type::SELL, seller_quantity-transaction_quantity, action_id, seller_trigger_type, seller_price, seller_trigger_price_lower, seller_trigger_price_upper, seller_expiration_time_date, seller_expiration_time_daily);
                        Sell_Orders[action_id].push_back(std::make_unique<Order>(new_seller_order_id, Database));
                        is_new_order_added = true;
                    }

                    // update order quantities before checking completion
                    buy_order->set_quantity(buyer_quantity - transaction_quantity);
                    sell_order->set_quantity(seller_quantity - transaction_quantity);

                    // if buy order is fully executed, move to the next one
                    if (buy_order->get_quantity() == 0){
                        break;
                    }

                }
                sell_it++;
            }
            buy_it++;
        }

        // remove completed orders
        orders.erase(std::remove_if(orders.begin(), orders.end(), [](const std::unique_ptr<Order>& order){return order->get_quantity() == 0;}), orders.end());
        sell_orders_for_action.erase(std::remove_if(sell_orders_for_action.begin(), sell_orders_for_action.end(), [](const std::unique_ptr<Order>& order){return order->get_quantity() == 0;}), sell_orders_for_action.end());
    }

    // if new orders were added, we call the function again to process them
    if (is_new_order_added){
        process_continuous_trading();
    }
}


// string representation methods 
// get the orders info as a string : order_time_date order_time_daily client_name order_type quantity action_name trigger_type price trigger_price_lower trigger_price_upper expiration_time_date expiration_time_daily,... (BUY then SELL orders)
std::string Market::get_orders_info() const
{    
    // getting the buy orders
    std::string buy_query = R"(SELECT o.order_time_date, o.order_time_daily, c.name, o.order_type, o.quantity, a.name, o.trigger_type, o.price, o.trigger_price_lower, o.trigger_price_upper, o.expiration_time_date, o.expiration_time_daily 
                            FROM orders o JOIN actions a ON o.action_id = a.action_id JOIN clients c ON o.client_id = c.client_id
                            WHERE o.order_type = 'BUY' AND o.order_status = 'PENDING')";
                            // ORDER BY o.price DESC, o.time DESC)";
    std::vector<std::vector<std::string>> buy_orders_info = Database.execute_SQL_query_vec_strings(buy_query);

    // getting the sell orders
    std::string sell_query = R"(SELECT o.order_time_date, o.order_time_daily, c.name, o.order_type, o.quantity, a.name, o.trigger_type, o.price, o.trigger_price_lower, o.trigger_price_upper, o.expiration_time_date, o.expiration_time_daily 
                            FROM orders o JOIN actions a ON o.action_id = a.action_id JOIN clients c ON o.client_id = c.client_id
                            WHERE o.order_type = 'SELL' AND o.order_status = 'PENDING')";
                            // ORDER BY o.price ASC, o.time DESC)";
    std::vector<std::vector<std::string>> sell_orders_info = Database.execute_SQL_query_vec_strings(sell_query);

    // join the buy and sell orders info into a single string
    std::string result;
    // process buy orders
    if (!buy_orders_info.empty()){
        for (int i=0; i<buy_orders_info.size(); i++){
            if (buy_orders_info[i].size() >= 12){
                result += fmt::format(
                    "{} {} {} {} {} {} {} {} {} {},",
                    two_times_to_string(std::stoll(buy_orders_info[i][0]), std::stoll(buy_orders_info[i][1])),
                    buy_orders_info[i][2],
                    buy_orders_info[i][3],
                    buy_orders_info[i][4],
                    buy_orders_info[i][5],
                    buy_orders_info[i][6],
                    buy_orders_info[i][7],
                    buy_orders_info[i][8],
                    buy_orders_info[i][9],
                    two_times_to_string(std::stoll(buy_orders_info[i][10]), std::stoll(buy_orders_info[i][11]))
                );
            }
        }
    }
    // process sell orders
    if (!sell_orders_info.empty()){
        for (int i=0; i<sell_orders_info.size(); i++){
            if (sell_orders_info[i].size() >= 12){
                result += fmt::format(
                    "{} {} {} {} {} {} {} {} {} {},",
                    two_times_to_string(std::stoll(sell_orders_info[i][0]), std::stoll(sell_orders_info[i][1])),
                    sell_orders_info[i][2],
                    sell_orders_info[i][3],
                    sell_orders_info[i][4],
                    sell_orders_info[i][5],
                    sell_orders_info[i][6],
                    sell_orders_info[i][7],
                    sell_orders_info[i][8],
                    sell_orders_info[i][9],
                    two_times_to_string(std::stoll(sell_orders_info[i][10]), std::stoll(sell_orders_info[i][11]))
                );
            }
        }
    }
    if (!result.empty()){
        result.pop_back(); // remove trailing comma
    }
    return result;
}

// get the actions info as a string : action_name quantity last_price time,...
std::string Market::get_actions_info() const
{
    std::string query = R"(SELECT a.name, a.quantity, p.price, p.daily_time, p.date_time
        FROM actions a LEFT JOIN prices p ON a.action_id = p.action_id
        WHERE (p.date_time, p.daily_time) = (
            SELECT p2.date_time, p2.daily_time
            FROM prices p2
            WHERE p2.action_id = a.action_id
            AND p2.date_time = (
                SELECT MAX(date_time)
                FROM prices
                WHERE action_id = p2.action_id
            )
            AND p2.daily_time = (
                SELECT MAX(daily_time)
                FROM prices
                WHERE action_id = p2.action_id
                AND date_time = p2.date_time
            )
        )
    )";
    std::vector<std::vector<std::string>> market_action_info = Database.execute_SQL_query_vec_strings(query);

    // check if we have enough data before accessing elements
    if (market_action_info.empty() || market_action_info[0].size() < 5){
        return "";
    }

    // join the action info into a single string
    std::string result;
    for (const auto& action_info : market_action_info){
        if (action_info.size() >= 5){
            result += fmt::format(
                "{} {} {} {},",
                action_info[0],
                action_info[1], 
                action_info[2],
                two_times_to_string(std::stoll(action_info[3]), std::stoll(action_info[4]))
            );
        }
    }
    if (!result.empty()){
        result.pop_back(); // remove trailing comma
    }
    return result;
}

// get the market info as a string : market_value;order_time_date order_time_daily client_name order_type quantity action_name trigger_type price trigger_price_lower trigger_price_upper expiration_time_date expiration_time_daily,... (BUY then SELL orders);action_name quantity last_price time,...
std::string Market::get_market_info() const
{
    std::string result = fmt::format(
        "{};{};{}",
        get_market_value(), 
        get_orders_info(), 
        get_actions_info()  
    );
    return result;
}

