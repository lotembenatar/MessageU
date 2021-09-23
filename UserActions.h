#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <map>

typedef void (*func_ptr)();

class UserActions
{
private:
    // Static class
    UserActions() {};
    ~UserActions() {};

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

    // Helper functions
    static std::map<std::string, func_ptr> create_client_action_map();
    static bool create_me_info_file(const std::string& username, uint64_t uuid);

public:
    // Display the usage
    static void display_usage();

    // Get input from user and map the correct function
    static void get_action_from_user();
};

