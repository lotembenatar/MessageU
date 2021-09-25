#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <map>

// This class encapsulate the functionality of the application
class ConsoleApp
{
private:
    typedef void (ConsoleApp::* func_ptr)();

    // One-to-one mapping between user input and function to execute
    std::map<std::string, func_ptr> client_actions;

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
    bool create_me_info_file(const std::string& username, uint64_t uuid);

    // Display the usage
    void display_usage();

    // Get input from user, map the correct function and execute it
    void get_action_from_user();
public:
    ConsoleApp();

    // Starts the application
    void start();
};

