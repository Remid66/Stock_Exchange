//=======================================================================
// File containing the definition of what a market is
//=======================================================================
#ifndef MARKET_HPP
#define MARKET_HPP
#include "database_management.hpp"


#include "client.hpp"
#include "messages.hpp"


class Market
{
private:
    // map where the key is the action id and the value is a vector of orders related to this action
    std::unordered_map<ID, std::vector<std::unique_ptr<Order>>> Buy_Orders; // buy orders for each action (refered by the action id)
    std::unordered_map<ID, std::vector<std::unique_ptr<Order>>> Sell_Orders; // sell orders for each action (refered by the action id)
    double Exchange_Price; // price of the transaction
    Database_Manager& Database; // reference to the database manager for queries (actions and clients)

public:
    // constructor
    Market(Database_Manager& database); // simple init
    Market(const Market&) = delete; // delete copy constructor
    Market& operator=(const Market&) = delete; // delete copy assignment operator
    Market(Market&& other) noexcept; // implement move constructor
    Market& operator=(Market&& other) noexcept; // implement move assignment operator

    // getters
    Database_Manager& get_database() const;

    // clients handling
    void deposit(const ID& client_id, const double& amount); // deposit funds into the account of a client
    void withdraw(const ID& client_id, const double& amount); // withdraw funds from the account of a client
    bool can_afford(const ID& client_id, const int& quantity, const double& price, const ID& action_id) const; // returns True if the amount can be withdrawn from the client balance
    bool has_shares(const ID& client_id, const ID& action_id, const int& quantity) const; // returns True if the action can be removed from the portfolio of the client
    bool client_exists(const ID& client_id) const; // check if a client exists
    bool client_name_exists(const std::string& client_name) const; // check if a client exists with the given name
    ID client_id_if_name_and_password_registered(const std::string& client_name, const std::string& client_password); // check if a client is registered with the given name and password and return its ID
    void add_client(const ID& client_id, const std::string& client_name, const std::string& client_password, const double& balance, std::unordered_map<ID, int> portfolio); // add a client to the market with its balance and portfolio (action_id and quantity)
    void remove_client(const ID& client_id); // remove a client from the market
    ID get_client_id_from_name(const std::string& client_name) const; // get the client id from a client name
    void update_client_portfolio(const ID& client_id, const Order_Type& order_type, const ID& action_id, const int& quantity, const double& price, const ID& daily_time, const ID& date_time); // update the portfolio of a client with a new action
    void add_order_to_client_completed_orders(const ID& client_id, const ID& order_id, const ID& order_time_date, const ID& order_time_daily, const Order_Type& order_type, const int& quantity, const ID& action_id, const Order_Trigger& trigger_type, const double& price, const double& trigger_price_lower, const double& trigger_price_upper, const ID& expiration_time_date, const ID& expiration_time_daily); // add an order to the completed orders of a client
    void add_order_to_client_pending_orders(const ID& client_id, const ID& order_id, const ID& order_time_date, const ID& order_time_daily, const Order_Type& order_type, const int& quantity, const ID& action_id, const Order_Trigger& trigger_type, const double& price, const double& trigger_price_lower, const double& trigger_price_upper, const ID& expiration_time_date, const ID& expiration_time_daily); // add an order to the pending orders of a client
    void remove_order_from_client_pending_orders(const ID& client_id, const ID& order_id); // remove an order from the pending orders of a client

    // actions handling
    bool action_exists(const ID& action_id) const; // check if an action exists
    void add_action(const ID& action_id, const std::string& name, const int& quantity, const double& price, const ID& daily_time, const ID& date_time); // add an action to the market
    void remove_action(const ID& action_id); // remove an action from the market
    double get_market_value() const; // get the market value (sum of the values of all the actions)

    // market functionment
    void accumulate_order(const ID& client_id, const ID& order_id, const ID& order_time_date, const ID& order_time_daily, const Order_Type& order_type, const int& quantity, const ID& action_id, const Order_Trigger& trigger_type, const double& price, const double& trigger_price_lower, const double& trigger_price_upper, const ID& expiration_time_date, const ID& expiration_time_daily); // accumulate an order to the market and sort the orders by priority (add the order to the pending orders for the client)    
    void deaccumulate_order(const ID& client_id, const ID& order_id,  const Order_Type& order_type, const ID& action_id); // remove an order from the pending orders of the client (if it exists) and remove it from the market orders by making again the market sorting
    void process_fixing(); // process the fixing of the price to order the transactions by priority
    void process_continuous_trading(); // process the continuous trading of the market, transactions between buyers and sellers of different actions

    // string representation methods
    std::string get_orders_info() const; // get the orders info as a string : order_time_date order_time_daily client_name order_type quantity action_name trigger_type price trigger_price_lower trigger_price_upper expiration_time_date expiration_time_daily,... (BUY then SELL orders)
    std::string get_actions_info() const; // get the actions info as a string : action_name quantity last_price time,...
    std::string get_market_info() const; // get the market info as a string : market_value;order_time_date order_time_daily client_name order_type quantity action_name trigger_type price trigger_price_lower trigger_price_upper expiration_time_date expiration_time_daily,... (BUY then SELL orders);action_name quantity last_price time,...
};


#endif // MARKET_HPP