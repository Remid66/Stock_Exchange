#include "database_management.hpp"
#include "market.hpp"


bool is_continuous_trading_period = false; // indicator for continuous trading period
std::mutex mtx;  // mutex for shared resources (e.g., Stock_Market)
bool processing = false; // flag to prevent multiple processing at the same time
std::atomic<bool> shutdown_flag(false); // global flag to stop client threads


// Function to handle client requests
void handle_client(int client_socket, Market& stock_market, const int& process_time)
{
    ID order_id = -1;
    char buffer[BUFFER_SIZE];

    while (!shutdown_flag.load()){
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0){
            break; // client disconnected (maybe the monitor_server function detected it)
        }
        std::string input(buffer);
        std::cout << "Client input : " << input << std::endl;
        
        const std::string authentification_prefix = "Authentification Request: ";
        // Check if the message starts with the expected prefix
        if (input.rfind(authentification_prefix, 0) == 0){
            // Extract username and password
            std::istringstream ss(input.substr(authentification_prefix.size()));
            std::string username, password;
            if (!(ss >> username >> password)){
                send(client_socket, "AUTHENTIFICATION_FAILURE_INPUT", strlen("AUTHENTIFICATION_FAILURE_INPUT"), 0);
                Message authentification_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                authentification_error_message.log_message(
                    0, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::AUTHENTIFICATION_FAILURE_INPUT, 
                    "Invalid input", 
                    get_current_time_ms()
                );
                break;
            }
            ID potential_client_id = stock_market.client_id_if_name_and_password_registered(username, encrypt_AES(password, key, iv));
            if (potential_client_id != -1){
                std::string response = fmt::format("AUTHENTIFICATION_SUCCESS {}", potential_client_id);
                send(client_socket, response.c_str(), response.length(), 0);
                Message authentification_success_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                authentification_success_message.log_message(
                    potential_client_id,
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::AUTHENTIFICATION_SUCCESS, 
                    "Authentification success", 
                    get_current_time_ms()
                );
            } 
            else {
                if (stock_market.client_name_exists(username)){
                    send(client_socket, "AUTHENTIFICATION_FAILURE_PASSWORD", strlen("AUTHENTIFICATION_FAILURE_PASSWORD"), 0);
                    ID client_id_without_password = stock_market.get_client_id_from_name(username);
                    Message authentification_failure_password_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                    authentification_failure_password_message.log_message(
                        client_id_without_password, 
                        Message::Sender::SERVER_MESSAGE, 
                        Message::Type::AUTHENTIFICATION_FAILURE_PASSWORD, 
                        "Wrong password", 
                        get_current_time_ms()
                    );
                } 
                else {
                    send(client_socket, "AUTHENTIFICATION_FAILURE_USERNAME", strlen("AUTHENTIFICATION_FAILURE_USERNAME"), 0);
                    Message authentification_failure_username_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                    authentification_failure_username_message.log_message(
                        0, 
                        Message::Sender::SERVER_MESSAGE, 
                        Message::Type::AUTHENTIFICATION_FAILURE_USERNAME, 
                        "Unknown username", 
                        get_current_time_ms()
                    );
                }
            }
            continue;
        }
        const std::string client_connected_prefix = "CLIENT_CONNECTED";
        if (input.find(client_connected_prefix) != std::string::npos){
            std::istringstream iss(input);
            ID client_id;
            std::string command; // = "CLIENT_CONNECTED"
            iss >> client_id >> command;
            Message client_connection(stock_market.get_database().get_new_message_id(), stock_market.get_database());
            client_connection.log_message(
                client_id, 
                Message::Sender::SERVER_MESSAGE, 
                Message::Type::CLIENT_CONNECTED, 
                "Client connected", 
                get_current_time_ms()
            );
            continue;
        }
        const std::string exit_keyword = "exit";
        if (input.find(exit_keyword) != std::string::npos){
            std::istringstream iss(input);
            ID client_id;
            std::string command; // = "exit"
            iss >> client_id >> command;
            Message client_disconnection(stock_market.get_database().get_new_message_id(), stock_market.get_database());
            client_disconnection.log_message(
                client_id,
                Message::Sender::SERVER_MESSAGE, 
                Message::Type::CLIENT_DISCONNECTED, 
                "Client disconnected", 
                get_current_time_ms()
            );
            break;
        }
        // display the orders if the user types 'display'
        const std::string display_keyword = "display";
        if (input.find(display_keyword) != std::string::npos){
            std::istringstream iss(input);
            ID client_id;
            std::string command; // = "display"
            std::string display_type; // = "portfolio/pending_orders/completed_orders/market/action_name"
            iss >> client_id >> command >> display_type;
            bool no_action_name_found = false;
            if (display_type == "portfolio"){
                Client client(client_id, stock_market.get_database());
                std::string response = client.get_portfolio_info();
                if (response.empty()){
                    response = "Empty portfolio";
                }
                send(client_socket, response.c_str(), response.length(), 0);
                Message display_portfolio_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                display_portfolio_message.log_message(
                    client_id,
                    Message::Sender::CLIENT_MESSAGE, 
                    Message::Type::DISPLAY_PORTFOLIO, 
                    "Display portfolio", 
                    get_current_time_ms()
                );
            }
            else if (display_type == "pending_orders"){
                Client client(client_id, stock_market.get_database());
                std::string response = client.get_pending_orders_info();
                if (response.empty()){
                    response = "No pending orders";
                }
                send(client_socket, response.c_str(), response.length(), 0);
                Message display_pending_orders_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                display_pending_orders_message.log_message(
                    client_id,
                    Message::Sender::CLIENT_MESSAGE, 
                    Message::Type::DISPLAY_PENDING_ORDERS, 
                    "Display pending orders", 
                    get_current_time_ms());
            }
            else if (display_type == "completed_orders"){
                Client client(client_id, stock_market.get_database());
                std::string response = client.get_completed_orders_info();
                if (response.empty()){
                    response = "No completed orders";
                }
                send(client_socket, response.c_str(), response.length(), 0);
                Message display_completed_orders_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                display_completed_orders_message.log_message(
                    client_id,
                    Message::Sender::CLIENT_MESSAGE, 
                    Message::Type::DISPLAY_COMPLETED_ORDERS, 
                    "Display completed orders", 
                    get_current_time_ms());
            }
            else if (display_type == "market"){
                std::string response = stock_market.get_market_info();
                send(client_socket, response.c_str(), response.length(), 0);
                Message display_market_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                display_market_message.log_message(client_id,
                    Message::Sender::CLIENT_MESSAGE, 
                    Message::Type::DISPLAY_MARKET, "Display market", get_current_time_ms());
            }
            else if (display_type != ""){ // an action's name is the display type
                // getting the action id from the name
                std::string query = fmt::format(
                    "SELECT action_id FROM actions WHERE name = '{}'", 
                    display_type
                );
                ID action_id = stock_market.get_database().execute_SQL_query_ID(query);
                if (action_id != -1){ // if we found it 
                    Action action(action_id, stock_market.get_database());
                    std::string response = action.get_action_info();
                    send(client_socket, response.c_str(), response.length(), 0);
                    Message display_action_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                    display_action_message.log_message(
                        client_id,
                        Message::Sender::CLIENT_MESSAGE, 
                        Message::Type::DISPLAY_ACTION, 
                        "Display action: " + display_type, 
                        get_current_time_ms()
                    );
                }
                else {
                    no_action_name_found = true;
                }
            }
            if (no_action_name_found){
                std::string response = fmt::format(
                    "Error: Display type '{}' not recognized, or action does not exist", 
                    display_type
                );
                send(client_socket, response.c_str(), response.length(), 0);
                Message display_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                display_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, "Display type not recognized", 
                    get_current_time_ms()
                );
            }
            continue;
        }
        // if the input contains a deposit command, we deposit the amount to the client
        if (input.find("deposit") != std::string::npos){ // "amount deposit"
            std::istringstream iss(input);
            int client_id;
            double amount;
            iss >> client_id >> amount;
            if (stock_market.client_exists(client_id)){
                stock_market.deposit(client_id, amount);
                std::string response = fmt::format("Deposited {}$ to client {}", amount, client_id);
                send(client_socket, response.c_str(), response.length(), 0);
                Message deposit_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                deposit_message.log_message(
                    client_id, 
                    Message::Sender::CLIENT_MESSAGE, 
                    Message::Type::DEPOSIT, 
                    response, 
                    get_current_time_ms());
            }
            else {
                std::string response = fmt::format("Client {} does not exist", client_id);
                send(client_socket, response.c_str(), response.length(), 0);
                Message client_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                client_error_message.log_message(
                    0, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    response, 
                    get_current_time_ms()
                );
            }
            continue;
        }
        // if the input contains a withdraw command, we withdraw the amount from the client
        if (input.find("withdraw") != std::string::npos){ // "amount withdraw"
            std::istringstream iss(input);
            int client_id;
            double amount;
            iss >> client_id >> amount;
            if (stock_market.client_exists(client_id)){
                if (stock_market.can_afford(client_id, 1, amount, -1)){
                    stock_market.withdraw(client_id, amount);
                    std::string response = fmt::format("Withdrew {}$ from client {}", amount, client_id);
                    send(client_socket, response.c_str(), response.length(), 0);
                    Message withdraw_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                    withdraw_message.log_message(
                        client_id, 
                        Message::Sender::CLIENT_MESSAGE, 
                        Message::Type::WITHDRAW, 
                        response, 
                        get_current_time_ms()
                    );
                } 
                else {
                    std::string response = fmt::format("Insufficient balance for client {} to withdraw {}", client_id, amount);
                    send(client_socket, response.c_str(), response.length(), 0);
                    Message withdraw_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                    withdraw_error_message.log_message(
                        client_id, 
                        Message::Sender::SERVER_MESSAGE,
                        Message::Type::ERROR, 
                        response, 
                        get_current_time_ms()
                    );
                }
            } 
            else {
                std::string response = fmt::format("Client {} does not exist", client_id);
                send(client_socket, response.c_str(), response.length(), 0);
                Message client_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                client_error_message.log_message(
                    0, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    response,
                    get_current_time_ms()
                );
            }
            continue;
        }

        // if the input contains an order, we then process it
        std::istringstream iss(input);
        std::string type_str, trigger_type_str, validity_date_str, validity_daily_time_str;
        ID client_id, action_id;
        int quantity;
        double price, trigger_price_lower, trigger_price_upper; 
        iss >> client_id 
            >> type_str 
            >> quantity
            >> action_id
            >> trigger_type_str;

        // verifying that this is a BUY or SELL order 
        Order_Type type = Order_Type::BUY;
        if (type_str == "SELL"){
            type = Order_Type::SELL;
        }
        else if (type_str != "BUY"){
            std::string response = "Error: Order type not recognized (use BUY or SELL)";
            send(client_socket, response.c_str(), response.length(), 0);
            Message order_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
            order_error_message.log_message(
                client_id, 
                Message::Sender::SERVER_MESSAGE, 
                Message::Type::ERROR, 
                "Order type not recognized", 
                get_current_time_ms()
            );
            continue;
        }

        // parsing of the input based on the order type 
        // a market order has no trigger price, so we set it to 0 and max_number
        // on top of that, the buy price is the selling price, and the order is prioritized compared to limit orders
        // so we set the price of the order to the maximum
        Order_Trigger trigger_type;
        if (trigger_type_str == "MARKET") {
            trigger_type = Order_Trigger::MARKET;
            price = max_number; // we set the price to the maximum, so it is prioritized, but the client will not be charged this price, but the selling price
            trigger_price_lower = 0.0;
            trigger_price_upper = max_number; 
        }
        // if the trigger type is a limit order, we set the max possible price to the maximum number
        // the lower bound is supposed to be provided by the client 
        else if (trigger_type_str == "LIMIT") {
            trigger_type = Order_Trigger::LIMIT;
            iss >> price >> trigger_price_lower;
            trigger_price_upper = max_number;
            // we must verify that a price has been given 
            if (!price){
                std::string response = "Error: Price must be provided for a limit order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message price_missing_error(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                price_missing_error.log_message(
                    client_id,
                    Message::Sender::SERVER_MESSAGE,
                    Message::Type::ERROR,
                    "Price must be provided for a limit order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the price is not negative
            if (price < 0.0){
                std::string response = "Error: Price must be positive for a limit order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message price_negative_error(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                price_negative_error.log_message(
                    client_id,
                    Message::Sender::SERVER_MESSAGE,
                    Message::Type::ERROR,
                    "Price must be positive for a limit order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the lower bound given exists
            if (!trigger_price_lower){
                std::string response = "Error: Trigger price lower must be provided for a limit order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price lower must be provided for a limit order", 
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the lower bound given is not negative
            if (trigger_price_lower < 0.0){
                std::string response = "Error: Trigger price lower must be positive for a limit order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price lower must be positive for a limit order", 
                    get_current_time_ms()
                );
                continue;
            }
        }
        // if the trigger type is a stop order, we set the lower bound to 0
        // the upper bound is supposed to be provided by the client
        else if (trigger_type_str == "STOP"){
            trigger_type = Order_Trigger::STOP;
            iss >> price >> trigger_price_upper;
            trigger_price_lower = 0.0;
            // we must verify that a price has been given 
            if (!price){
                std::string response = "Error: Price must be provided for a stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message price_missing_error(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                price_missing_error.log_message(
                    client_id,
                    Message::Sender::SERVER_MESSAGE,
                    Message::Type::ERROR,
                    "Price must be provided for a stop order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the price is not negative
            if (price < 0.0){
                std::string response = "Error: Price must be positive for a stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message price_negative_error(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                price_negative_error.log_message(
                    client_id,
                    Message::Sender::SERVER_MESSAGE,
                    Message::Type::ERROR,
                    "Price must be positive for a stop order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the upper bound given exists
            if (!trigger_price_upper){
                std::string response = "Error: Trigger price upper must be provided for a stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price upper must be provided for a stop order",    
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the upper bound given is not negative
            if (trigger_price_upper < 0.0){
                std::string response = "Error: Trigger price upper must be positive for a stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price upper must be positive for a stop order",
                    get_current_time_ms()
                );
                continue;   
            }
        }
        else if (trigger_type_str == "LIMIT_STOP") {
            trigger_type = Order_Trigger::LIMIT_STOP;
            iss >> price >> trigger_price_lower >> trigger_price_upper;
            // we must verify that a price has been given 
            if (!price){
                std::string response = "Error: Price must be provided for a limit-stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message price_missing_error(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                price_missing_error.log_message(
                    client_id,
                    Message::Sender::SERVER_MESSAGE,
                    Message::Type::ERROR,
                    "Price must be provided for a limit-stop order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the price is not negative
            if (price < 0.0){
                std::string response = "Error: Price must be positive for a limit-stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message price_negative_error(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                price_negative_error.log_message(
                    client_id,
                    Message::Sender::SERVER_MESSAGE,
                    Message::Type::ERROR,
                    "Price must be positive for a limit-stop order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the lower bound given exists
            if (!trigger_price_lower){
                std::string response = "Error: Trigger price lower must be provided for a limit stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price lower must be provided for a limit stop order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the lower bound given is not negative
            if (trigger_price_lower < 0.0){
                std::string response = "Error: Trigger price lower must be positive for a limit stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price lower must be positive for a limit stop order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the upper bound given exists
            if (!trigger_price_upper){
                std::string response = "Error: Trigger price upper must be provided for a limit stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price upper must be provided for a limit stop order",
                    get_current_time_ms()
                );
                continue;
            }
            // we must verify that the upper bound given is not negative
            if (trigger_price_upper < 0.0){
                std::string response = "Error: Trigger price upper must be positive for a limit stop order";
                send(client_socket, response.c_str(), response.length(), 0);
                Message trigger_price_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                trigger_price_error_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Trigger price upper must be positive for a limit stop order",
                    get_current_time_ms()
                );
                continue;
            }
        }
        // otherwise there has been a mistake made by the client considering the input to pass an order 
        else {
            std::string response = "Error: Trigger type not recognized (use MARKET, LIMIT, STOP or LIMIT_STOP)";
            send(client_socket, response.c_str(), response.length(), 0);
            Message trigger_type_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
            trigger_type_error_message.log_message(
                client_id, 
                Message::Sender::SERVER_MESSAGE, 
                Message::Type::ERROR, 
                "Trigger type not recognized", 
                get_current_time_ms()
            );
            continue;
        }

        // extracting a possible validity date entered by the client
        iss >> validity_date_str >> validity_daily_time_str;
        ID validity_date = max_number;
        if (!validity_date_str.empty()){
            validity_date = get_date_id_from_string(validity_date_str);
        }
        ID validity_daily = 0;
        if (!validity_daily_time_str.empty()){
            validity_daily = get_daily_id_from_string(validity_daily_time_str);
        }

        // looking if the action exists, if not we said it 
        if (!stock_market.action_exists(action_id)){
            std::string response = fmt::format("Error: Action {} does not exist", action_id);
            send(client_socket, response.c_str(), response.length(), 0);
            Message action_error_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
            action_error_message.log_message(
                client_id, 
                Message::Sender::SERVER_MESSAGE, 
                Message::Type::ERROR, 
                response, 
                get_current_time_ms()
            );
            continue;
        }

        // before accumulating the order, we need to check if the client has enough funds or actions
        if (type == Order_Type::BUY){
            if (!stock_market.can_afford(client_id, quantity, price, action_id)){
                std::string response = "Error: Insufficient balance for buying";
                send(client_socket, response.c_str(), response.length(), 0);
                Message client_insufficient_balance_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                client_insufficient_balance_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Insufficient balance for buying", 
                    get_current_time_ms()
                );
                continue;
            }
        }
        else if (type == Order_Type::SELL){
            if (!stock_market.has_shares(client_id, action_id, quantity)){
                std::string response = "Error: Failed to sell action, client does not have enough shares";
                send(client_socket, response.c_str(), response.length(), 0);
                Message client_failed_to_sell_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
                client_failed_to_sell_message.log_message(
                    client_id, 
                    Message::Sender::SERVER_MESSAGE, 
                    Message::Type::ERROR, 
                    "Failed to sell action, client does not have enough shares", 
                    get_current_time_ms()
                );
                continue;
            }
        }

        // if the order is valid, we create it
        order_id = stock_market.get_database().get_new_order_id();
        Time order_time = get_current_time_ms();
        ID order_time_daily = get_daily_time(order_time);
        ID order_time_date = get_date_time(order_time);

        std::string response = fmt::format(
            "Order created with ID: {} for client {} to {} {} actions of {} at the price of {}$ at time {} with trigger type {} and trigger price lower {} and trigger price upper {} until validity date {}",
            order_id, 
            client_id, 
            type_str, 
            quantity, 
            action_id, 
            price,
            time_to_string(order_time), 
            trigger_type_str, 
            trigger_price_lower, 
            trigger_price_upper,
            two_times_to_string(validity_date, validity_daily)
        );
        Message order_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
        order_message.log_message(
            client_id, 
            Message::Sender::CLIENT_MESSAGE, 
            Message::Type::ORDER, 
            response, 
            order_time
        );
        send(client_socket, response.c_str(), response.length(), 0);

        // running the session by making the trades for an order without any trigger
        // otherwise it will be delayed until the trigger is reached (dealed with another thread and function)
        if (trigger_type == Order_Trigger::MARKET){
            stock_market.accumulate_order(client_id, order_id, order_time_date, order_time_daily, type, quantity, action_id, trigger_type, price, trigger_price_lower, trigger_price_upper, validity_date, validity_daily);
            Message server_accumulating_order_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
            server_accumulating_order_message.log_message(
                0, 
                Message::Sender::SERVER_MESSAGE, 
                Message::Type::ACCUMULATING_ORDER, 
                "Accumulating the order …", 
                get_current_time_ms()
            );
            if (is_continuous_trading_period){
                // if not already processing, process the trades
                if (!processing){
                    processing = true;
                    mtx.lock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(process_time)); // simulate processing time
                    stock_market.process_continuous_trading();
                    mtx.unlock();
                    processing = false;
                }
            }
        }
        // we still create the order in the database even if it is not processed yet
        else {
            stock_market.add_order_to_client_pending_orders(client_id, order_id, order_time_date, order_time_daily, type, quantity, action_id, trigger_type, price, trigger_price_lower, trigger_price_upper, validity_date, validity_daily);
        }
    }
    close(client_socket);
}


// this function will handle client connections concurrently
void accept_clients(int server_fd, struct sockaddr_in& client_addr, socklen_t& addr_len, Market& stock_market, const int& process_time) {
    fd_set read_fds;
    struct timeval timeout;

    while (!shutdown_flag.load()){
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        timeout.tv_sec = 1;  // timeout of 1 seconds
        timeout.tv_usec = 0;

        int activity = select(server_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity < 0 && errno != EINTR){
            perror("Select error");
            break;
        }

        if (activity > 0 && FD_ISSET(server_fd, &read_fds)){
            int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_socket < 0) {
                perror("Error accept");
                continue;
            }
            std::thread client_thread(handle_client, client_socket, std::ref(stock_market), process_time);
            client_thread.detach();
        }
    }
}


// handle the market session phases independently to the clients interactions
void market_session(Market& stock_market, int pre_open_time_delay, int open_time_delay, int continuous_trading_time_delay, int pre_close_time_delay, int continuous_trading_loop_duration)
{
    // pre-open phase: Accumulate orders without transactions
    std::cout << "Pre-open phase, accumulating orders …" << std::endl;
    Message pre_open_phase_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
    pre_open_phase_message.log_message(
        0, 
        Message::Sender::SERVER_MESSAGE, 
        Message::Type::PRE_OPEN_PHASE, 
        "Market pre-open phase, accumulating orders", 
        get_current_time_ms()
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(pre_open_time_delay));

    // open phase: Calculate equilibrium price (Price Fixing)
    std::cout << "Open phase, calculating equilibrium price …" << std::endl;
    stock_market.process_fixing(); // process the fixing of the price to execute the transactions possible and defining the equilibrium price
    Message open_phase_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
    open_phase_message.log_message(
        0, 
        Message::Sender::SERVER_MESSAGE, 
        Message::Type::OPEN_PHASE, 
        "Market open phase (fixing)", 
        get_current_time_ms()
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(open_time_delay));

    // continuous trading phase: Run stock market exchange in real time
    std::cout << "Continuous trading phase, running stock market exchange …" << std::endl;
    Message continuous_trading_phase_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
    continuous_trading_phase_message.log_message(
        0, 
        Message::Sender::SERVER_MESSAGE, 
        Message::Type::CONTINUOUS_TRADING_PHASE, 
        "Market continuous trading phase", 
        get_current_time_ms()
    );
    auto continuous_trading_end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(continuous_trading_time_delay);
    is_continuous_trading_period = true;
    while (std::chrono::steady_clock::now() < continuous_trading_end_time){
        std::this_thread::sleep_for(std::chrono::milliseconds(continuous_trading_loop_duration));
    }
    is_continuous_trading_period = false;

    // pre-close phase: Calculate equilibrium price (Price Fixing)
    std::cout << "Pre-close phase, accumulating orders …" << std::endl;
    Message pre_close_phase_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
    pre_close_phase_message.log_message(
        0, 
        Message::Sender::SERVER_MESSAGE, 
        Message::Type::PRE_CLOSE_PHASE, 
        "Market pre-close phase (fixing)", 
        get_current_time_ms()
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(pre_close_time_delay));

    // market closing phase: Market is closing, wrap up transactions
    std::cout << "Market closing phase, calculating equilibrium price …" << std::endl;
    stock_market.process_fixing(); // process the fixing of the price to execute the transactions possible and defining the equilibrium price
    Message close_phase_message(stock_market.get_database().get_new_message_id(), stock_market.get_database());
    close_phase_message.log_message(
        0, 
        Message::Sender::SERVER_MESSAGE, 
        Message::Type::CLOSE_PHASE,
        "Market close phase, accumulating orders", 
        get_current_time_ms()
    );

    shutdown_flag.store(true);
}


// function to get or generate if needed the encryption keys
void get_or_generate_crypted_keys(Database_Manager& stock_market_database)
{
    // trying to retrieve the encryption keys from the database
    std::string query_key = "SELECT key FROM encryption_keys";
    bool key_found = false;
    std::string query_iv = "SELECT iv FROM encryption_keys";
    bool iv_found = false;
    
    // retrieve the encryption key from the database
    std::vector<unsigned char> key_data = stock_market_database.execute_SQL_query_blob(query_key);
    if (key_data.size() == AES_KEY_SIZE){
        std::copy(key_data.begin(), key_data.end(), key);  // copy the retrieved key to the key array
        key_found = true;
    } 
    // retrieve the IV from the database
    std::vector<unsigned char> iv_data = stock_market_database.execute_SQL_query_blob(query_iv);
    if (iv_data.size() == AES_IV_SIZE){
        std::copy(iv_data.begin(), iv_data.end(), iv);  // copy the retrieved IV to the iv array
        iv_found = true;
    } 

    // if the key or IV is not found, generate a new key and IV
    if (!key_found || !iv_found){
        RAND_bytes(key, AES_KEY_SIZE);
        RAND_bytes(iv, AES_IV_SIZE);

        // prepare the insert query with placeholders for BLOB data
        std::string insert_key_iv_query = "INSERT INTO encryption_keys (key, iv) VALUES (?, ?)";
        // use prepared statement to insert the BLOB data
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(stock_market_database.get_database(), insert_key_iv_query.c_str(), -1, &stmt, nullptr) == SQLITE_OK){
            sqlite3_bind_blob(stmt, 1, key, AES_KEY_SIZE, SQLITE_STATIC); // bind the key BLOB
            sqlite3_bind_blob(stmt, 2, iv, AES_IV_SIZE, SQLITE_STATIC);  // bind the IV BLOB
            if (sqlite3_step(stmt) != SQLITE_DONE){
                std::cerr << "Error inserting key and iv into database: " << sqlite3_errmsg(stock_market_database.get_database()) << std::endl;
            }
        } 
        else {
            std::cerr << "Error preparing SQL insert statement: " << sqlite3_errmsg(stock_market_database.get_database()) << std::endl;
        }
        sqlite3_finalize(stmt); // finalize the statement
    }    
}


// function to log the forced client disconnections due to market closing
void log_others_client_disconnects(Market& stock_market, Time server_launch_time)
{
    // getting the clients ids that were connected but not disconnected 
    // (they could have connected, decomnected and reconnected, but not disconnected properly, and by the server shutdown)
    std::string client_disconnected_ids_query = fmt::format(
        R"(SELECT DISTINCT client_id FROM messages WHERE time > {} AND client_id NOT IN (
            SELECT client_id FROM messages WHERE time > {} 
            GROUP BY client_id HAVING 
                SUM(CASE WHEN message_type = 'CLIENT_CONNECTED' THEN 1 ELSE 0 END) =
                SUM(CASE WHEN message_type = 'CLIENT_DISCONNECTED' THEN 1 ELSE 0 END
        ))",
        server_launch_time,
        server_launch_time
    );
    std::vector<ID> client_ids = stock_market.get_database().execute_SQL_query_IDs(client_disconnected_ids_query);
    for (const auto& client_id : client_ids){
        Message client_disconnection(stock_market.get_database().get_new_message_id(), stock_market.get_database());
        client_disconnection.log_message(
            client_id, 
            Message::Sender::SERVER_MESSAGE, 
            Message::Type::CLIENT_DISCONNECTED, 
            "Client disconnected", 
            get_current_time_ms()
        );
    }
}


// function to display all the messages contained in the database
void display_all_messages(Market& stock_market)
{
    // display all the messages contained in the database by chronological order
    std::cout << "\n-------------- Displaying all messages in the database --------------\n";
    std::string messages_query = "SELECT message_id FROM Messages ORDER BY date_time ASC, daily_time ASC";
    std::vector<ID> message_ids = stock_market.get_database().execute_SQL_query_IDs(messages_query);
    for (const auto& message_id : message_ids){
        Message message(message_id, stock_market.get_database());
        message.display_message();
    }
    std::cout << "-------------- End of messages in the database --------------\n";
}


int main(int argc, char* argv[])
{
    // reset the launch time when the program starts
    Time server_launch_time = get_current_time_ms();
    ID server_launch_time_daily = get_daily_time(server_launch_time);
    ID server_launch_time_date = get_date_time(server_launch_time);

    // initialize the database with the market data
    Database_Manager Stock_Market_Database("../Data/Stock_Market_App.db");
    Market Stock_Market(Stock_Market_Database);

    // update or generate the encryption keys
    get_or_generate_crypted_keys(Stock_Market_Database);

    // handle command-line arguments
    if (argc <= 1){
        std::cerr << "Usage: " << argv[0] << " [init|reset|reset_prices|play]\n";
        return EXIT_FAILURE;
    }
    std::string arg = argv[1];
    if (arg == "init"){
        // adding the init actions
        Stock_Market.add_action(1, "CAC40", 20, 10.0, server_launch_time_daily, server_launch_time_date);
        Stock_Market.add_action(2, "SP500", 10, 20.0, server_launch_time_daily, server_launch_time_date);

        // adding the init clients
        std::string password_1 = "123";
        std::string encrypted_password_1 = encrypt_AES(password_1, key, iv);
        Stock_Market.add_client(1, "Client1", encrypted_password_1, 1000.0,{});
        std::string password_2 = "123";
        std::string encrypted_password_2 = encrypt_AES(password_2, key, iv);
        Stock_Market.add_client(2, "Client2", encrypted_password_2, 100.0, {{1, 20}, {2, 10}});

        Stock_Market_Database.close_database(); // close the database
        return EXIT_SUCCESS;
    } 
    if (arg == "reset"){
        Stock_Market_Database.reset_database();
        // update or generate the encryption keys
        get_or_generate_crypted_keys(Stock_Market_Database);
        Stock_Market_Database.close_database(); // close the database
        return EXIT_SUCCESS;
    }
    if (arg == "reset_prices"){
        Stock_Market_Database.reset_database_action_prices(server_launch_time_daily, server_launch_time_date);
        Stock_Market_Database.close_database(); // close the database
        return EXIT_SUCCESS;
    } 
    // handle the play part there
    if (argc < 2 || std::string(argv[1]) != "play"){        
        std::cerr << "Usage: " << argv[0] << " play\n";
        return EXIT_FAILURE;
    }

    // create the server socket
    int server_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    if (bind(server_fd, (struct sockaddr*)&address, addr_len) < 0){
        perror("Error bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0){
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Waiting for connexion on the port " << SERVER_PORT << "...\n";

    std::cout << "Initial market state:\n";
    std::cout << Stock_Market.get_market_info() << std::endl;
    Client client1(1, Stock_Market_Database);
    std::cout << client1.get_portfolio_info() << std::endl;
    Client client2(2, Stock_Market_Database);
    std::cout << client2.get_portfolio_info() << std::endl;

    // adding to the message log that the server was launched
    Message server_launch(Stock_Market.get_database().get_new_message_id(), Stock_Market.get_database());
    server_launch.log_message(0, Message::Sender::SERVER_MESSAGE, Message::Type::SERVER_RESTART, "Server launched", server_launch_time);

    // define delays (in milliseconds)
    int pre_open_time_delay = 1000;                 // accumulate orders before the market opens
    int open_time_delay = 1000;                     // calculate the equilibrium price during market open
    int continuous_trading_time_delay = 30000;       // real-time stock market operations
    int continuous_trading_loop_duration = 1000;    // accumulate orders without transactions
    int pre_close_time_delay = 1000;                // calculate equilibrium price before market close
    int process_time = 500;                        // simulate processing time

    // start the market session in a separate thread
    std::thread market_thread(market_session, std::ref(Stock_Market), pre_open_time_delay, open_time_delay, continuous_trading_time_delay, pre_close_time_delay, continuous_trading_loop_duration);
    
    // start accepting clients concurrently
    std::thread accept_thread(accept_clients, server_fd, std::ref(address), std::ref(addr_len), std::ref(Stock_Market), process_time);

    // join the market thread to ensure the market session ends
    market_thread.join();

    // all client threads must stop after the market session ends, so we close the server socket
    std::cout << "Market session ended. Closing all client connections...\n";
    accept_thread.join(); // closing the server socket
    // adding the message to the log that the server is closing
    Message server_closing(Stock_Market.get_database().get_new_message_id(), Stock_Market.get_database());
    server_closing.log_message(
        0, 
        Message::Sender::SERVER_MESSAGE, 
        Message::Type::SERVER_SHUTDOWN, 
        "Server shutdown", 
        get_current_time_ms()
    );

    // getting the clients ids that were connected but not disconnected
    // (they could have connected, decomnected and reconnected, but not disconnected properly, and by the server shutdown)
    log_others_client_disconnects(Stock_Market, server_launch_time);

    // display all the messages contained in the database
    display_all_messages(Stock_Market);

    Stock_Market_Database.close_database(); // close the database
    close(server_fd);
    return EXIT_SUCCESS;
}
// command to use the main
/*
./server.x reset : to reset the database entirely
./server.x reset_prices : to reset the prices of the actions in the database to only the last price and the given time (suppressed the history of prices)
./server.x init : to initialize the database with the little by hand market
./server.x play : to play a session with the market
*/

