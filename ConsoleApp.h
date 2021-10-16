#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <map>

#include "Util.h"
#include "WinsockClient.h"

// This class encapsulate the functionality of the application
class ConsoleApp
{
    static constexpr uint8_t CLIENT_VERSION = 1;
    static constexpr const char ME_INFO_PATH[] = "me.info";
    typedef void (ConsoleApp::* func_ptr)();

    // One-to-one mapping between user input and function to execute
    const std::map<std::string, func_ptr> client_actions_map;

    // Object for sending requests to server
    WinsockClient winsock_client;

    // Client ID for this application
    // Should be initialized on startup or after registration
    std::vector<uint8_t> client_id;

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
    bool create_me_info_file(const std::string& username, const uint8_t* uuid, const uint8_t* public_key) const;
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

