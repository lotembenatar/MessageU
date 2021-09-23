#include <iostream>
#include <string>
#include <sstream>
#include <random>
#include <fstream>

#include "UserActions.h"
#include "WinsockClient.h"

int test_winsock_client() {
    WinsockClient winsock_client;
    server_request_header request_header{};
    server_response_header response_header{};
    std::vector<uint8_t> c_payload;
    std::vector<uint8_t> s_payload;

    std::cout << "Winsock client test started\n" << std::endl;
    std::cout << "request header:" << std::endl;
    std::cout << "Client ID: " << request_header.client_id << std::endl;
    std::cout << "Code: " << request_header.code << std::endl;
    std::cout << "Version: " << request_header.version << std::endl;
    std::cout << "Payload size: " << request_header.payload_size << std::endl;
    
    if (!winsock_client.send_request(request_header, c_payload, response_header, s_payload))
    {
        std::cerr << "Send request failed" << std::endl;
        return -1;
    }

    std::cout << "\nWinsock client functions was successful\n" << std::endl;
    std::cout << "Response header:" << std::endl;
    std::cout << "Code: " << response_header.code << std::endl;
    std::cout << "Version: " << response_header.version << std::endl;
    std::cout << "Payload size: " << response_header.payload_size << std::endl;

    return 0;
}

void start_app() {
    // Display usage
    UserActions::display_usage();

    while (true)
    {
        // User action
        UserActions::get_action_from_user();
    }
}

int main()
{
    //start_app();
    test_winsock_client();
    return 0;
}