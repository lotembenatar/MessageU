#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <map>

typedef void (*func_ptr)();

// This class encapsulate the functionality of the application
class ConsoleApp
{
private:
    // One-to-one mapping between user input and function to execute
    static const std::map<std::string, func_ptr> client_actions;

    // User mapped functions
    static void register_client();
    static void request_for_client_list();
    static void request_for_public_key();
    static void request_for_waiting_messages();
    static void send_text_message();
    static void send_request_for_symmetric_key();
    static void send_symmetric_key();
    static void send_file();
    static void exit_client();

    // This function to should initialize the const client actions map (input to function)
    static std::map<std::string, func_ptr> create_client_action_map();

    // Helper functions
    static bool create_me_info_file(const std::string& username, uint64_t uuid);

    // Display the usage
    static void display_usage();

    // Get input from user, map the correct function and execute it
    static void get_action_from_user();
public:
    // Static class
    ConsoleApp() = delete;
    ~ConsoleApp() = delete;

    // Starts the application
    static void start();
};

