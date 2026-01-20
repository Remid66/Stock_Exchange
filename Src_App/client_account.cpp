#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "database_management.hpp"


#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "database_management.hpp"


//IP TOM : "147.250.233.58"
//IP THOMAS : "147.250.227.175"
#define SERVER_IP "127.0.0.1"  // IP address of the server (can be localhost or a not that far distant server)
#define PORT 8080
#define BUFFER_SIZE 1024


std::atomic<bool> is_running(true); 


// function to check if the server is running by trying to connect
bool is_server_running()
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1){
        return false;  // socket creation failed, server is not running
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0){
        close(client_socket);
        return false; // invalid address
    }
    // try to connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        close(client_socket);
        return false;  // server is not running
    }

    close(client_socket);
    return true;
}

// function that monitors the server and exits if it stops
void monitor_server()
{
    while (is_running){
        if (!is_server_running()){
            std::cout << "\nServer is not running. Exiting...\n";
            is_running = false;
            exit(0); // terminate the client process immediately
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // check every second
    }
}


int main(int argc, char* argv[])
{
    std::cout << "Client launched...\n";
    std::thread server_monitor(monitor_server);
    // wait for the server to be available
    while (!is_server_running()){
        std::cout << "Waiting for the server...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
 
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    // socket creation
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error socket creation");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // IP address conversion
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0){
        perror("Unvalid IP address");
        exit(EXIT_FAILURE);
    }
    // server connection
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Error connection");
        exit(EXIT_FAILURE);
    }

    ID client_id = -1;
    // ask the server wether the client is already registered
    std::string message = "Authentification Request: " + std::string(argv[1]) + " " + std::string(argv[2]);
    send(sock, message.c_str(), message.length(), 0);
    memset(buffer, 0, BUFFER_SIZE);
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread <= 0){
        std::cout << "Connection closed by the server.\n";
        return EXIT_FAILURE;
    }

    // getting the response from the server
    if (argc < 3){
        std::cerr << "Usage: " << argv[0] << "<client_name> <password>\n";
        return EXIT_FAILURE;
    }
    std::string response(buffer);
    std::cout << "Serveur authentification response: " << response << std::endl;
    const std::string authentification_prefix = "AUTHENTIFICATION_SUCCESS";
    // Check if the message starts with the expected prefix
    if (response.rfind(authentification_prefix, 0) == 0){
        client_id = std::stoi(response.substr(authentification_prefix.size()));
    }
    else if (response == "AUTHENTIFICATION_FAILURE_INPUT"){
        std::cerr << "Error: Invalid input\n";
        return EXIT_FAILURE;
    }
    else if (response == "AUTHENTIFICATION_FAILURE_USERNAME"){
        std::cerr << "Error: Username not found\n";
        return EXIT_FAILURE;
    }
    else if (response == "AUTHENTIFICATION_FAILURE_PASSWORD"){
        std::cerr << "Error: Incorrect password\n";
        return EXIT_FAILURE;
    }
    std::cout << "Connected to the server !\n";
    std::string connection_message = std::to_string(client_id) + " CLIENT_CONNECTED";
    send(sock, connection_message.c_str(), connection_message.length(), 0);

    while (is_running){
        std::cout << "Enter one of the following commands:\n"
                << "1. Place an order:\n"
                << "   [BUY/SELL] [quantity] [action_id] [MARKET/LIMIT/STOP/LIMIT_STOP] [price] [trigger_price_lower] [trigger_price_upper] [validity_date: YYYY-MM-DD] [validity_time: HH:MM:SS]\n"
                << "   Notes:\n"
                << "     - For MARKET:        do NOT provide price, trigger_price_lower or trigger_price_upper\n"
                << "     - For LIMIT:         provide [price] and [trigger_price_lower] only\n"
                << "     - For STOP:          provide [price] and [trigger_price_upper] only\n"
                << "     - For LIMIT_STOP:    provide [price], [trigger_price_lower] and [trigger_price_upper]\n"
                << "\n"
                << "2. Modify client balance:\n"
                << "   amount [value] [deposit/withdraw]\n"
                << "\n"
                << "3. Display information:\n"
                << "   display [portfolio | pending_orders | completed_orders | market | action_name]\n"
                << "\n"
                << "4. Disconnect from server:\n"
                << "   exit\n"
                << "\n";

        std::string message;
        std::getline(std::cin, message);
        message = std::to_string(client_id) + " " + message;

        // exit conditions
        if (message == std::to_string(client_id) + " exit"){
            is_running = false;
            std::cout << "Exiting from user command...\n";
            break;
        }
        if (!is_running){
            std::cout << "Exiting from server monitor...\n";
            break;
        }

        // send request to the server
        send(sock, message.c_str(), message.length(), 0);

        // receive response from the server
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread <= 0){
            std::cout << "Connexion closed by the server.\n";
            break;
        }
        std::cout << "Server : " << buffer << std::endl;
    }

    close(sock);
    return 0;
}

