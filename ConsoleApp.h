#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <map>

#include "Util.h"
#include "WinsockClient.h"

#include "Base64Wrapper.h"
#include "RSAWrapper.h"
#include "AESWrapper.h"

struct Client {
    std::vector<uint8_t> uuid;
    std::string name;
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> session_key;
};

// This class encapsulate the functionality of the application
class ConsoleApp
{
    static constexpr uint8_t CLIENT_VERSION = 2;
    static constexpr const char ME_INFO_PATH[] = "me.info";
    typedef void (ConsoleApp::* func_ptr)();

    // One-to-one mapping between user input and function to execute
    const std::map<std::string, func_ptr> client_actions_map;

    // Object for sending requests to server
    WinsockClient winsock_client;

    // Client ID for this application
    // Should be initialized on startup or after registration
    std::vector<uint8_t> client_id;

    // Private key in base64 representation for current client
    std::string base64_private_key;

    // User name to client map - initialized in client list request
    std::map<std::string, Client> username_to_client_map;

    // User mapped functions
    void register_client();
    void request_for_client_list();
    void request_for_public_key();
    void request_for_waiting_messages();
    void send_text_message();
    void send_request_for_symmetric_key();
    void send_symmetric_key();
    void send_file();
    void exit_client();

    // Helper functions
    void send_message_to_client(ClientMessageType message_type); // Unify all message requests
    bool create_me_info_file(const std::string& username, const uint8_t* uuid) const;
    void load_me_info_file();
    bool is_registered();

    // Creates the user input to function map
    std::map<std::string, func_ptr> create_client_action_map();

    // Display the usage
    void display_usage();

    // Get input from user, map the correct function and execute it
    void get_action_from_user();
public:
    ConsoleApp();

    // Starts the application
    void start();
};

