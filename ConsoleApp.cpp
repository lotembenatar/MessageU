#include "ConsoleApp.h"

#define NOT_IMPLEMENTED {std::cout << __FUNCTION__ << " not implemented" << std::endl;}

void ConsoleApp::register_client()
{
    std::cout << "Please enter registration user name:" << std::endl;

    // Get username from client
    std::string username;
    std::getline(std::cin, username);

    // TODO: Send registration request to server
    uint64_t uuid = 1234;
    std::cout << "Registering with username " << username << " and UUID " << uuid << " ..." << std::endl;

    // Save username and uuid in me.info
    if (create_me_info_file(username, uuid)) {
        std::cout << "Registeration done." << std::endl;
    }
    else
    {
        std::cerr << "Registeration failed - user already exists" << std::endl;
    }
}

void ConsoleApp::request_for_client_list()
{
    NOT_IMPLEMENTED;
}

void ConsoleApp::request_for_public_key()
{
    NOT_IMPLEMENTED;
}

void ConsoleApp::request_for_waiting_messages()
{
    NOT_IMPLEMENTED;
}

void ConsoleApp::send_text_message()
{
    NOT_IMPLEMENTED;
}

void ConsoleApp::send_request_for_symmetric_key()
{
    NOT_IMPLEMENTED;
}

void ConsoleApp::send_symmetric_key()
{
    NOT_IMPLEMENTED;
}

void ConsoleApp::send_file()
{
    NOT_IMPLEMENTED;
}

void ConsoleApp::exit_client()
{
    std::cout << "Bye bye!" << std::endl;
    exit(0);
}

ConsoleApp::ConsoleApp()
{
    client_actions = {
       {"10" , &ConsoleApp::register_client},
       {"20" , &ConsoleApp::request_for_client_list},
       {"30" , &ConsoleApp::request_for_public_key},
       {"40" , &ConsoleApp::request_for_waiting_messages},
       {"50" , &ConsoleApp::send_text_message},
       {"51" , &ConsoleApp::send_request_for_symmetric_key},
       {"52" , &ConsoleApp::send_symmetric_key},
       {"53" , &ConsoleApp::send_file},
       {"0" , &ConsoleApp::exit_client},
    };
}

// TODO - add public key
bool ConsoleApp::create_me_info_file(const std::string& username, uint64_t uuid)
{
    const char* filepath = "me.info";

    // Check if already exists
    if (std::filesystem::exists(filepath)) {
        return false;
    }

    // Create the file using file stream
    std::ofstream fileStream(filepath);

    // Write username
    fileStream.write(username.c_str(), username.length());
    fileStream.write("\n", 1);
    // Write UUID
    std::stringstream stream;
    stream << std::hex << uuid;
    std::string uuid_as_str(stream.str());
    fileStream.write(uuid_as_str.c_str(), uuid_as_str.length());

    // close stream
    fileStream.close();

    return true;
}

void ConsoleApp::display_usage()
{
    std::cout << "MessageU client at your service.\n";
    std::cout << "10) Register\n";
    std::cout << "20) Request for clients list\n";
    std::cout << "30) Request for public key\n";
    std::cout << "40) Request for waiting messages\n";
    std::cout << "50) Send a text message\n";
    std::cout << "51) Send a request for symmetric key\n";
    std::cout << "52) Send your symmetric key\n";
    std::cout << "53) Send a file\n";
    std::cout << "0) Exit client\n";
    std::cout << std::endl; // drop line and flush buffer
}

void ConsoleApp::start()
{
    // Display usage
    display_usage();

    while (true)
    {
        // Get user action and execute it
        get_action_from_user();
    }
}

void ConsoleApp::get_action_from_user()
{
    // Get user input
    std::string action;
    std::getline(std::cin, action);

    // Check if user input exists in our map
    auto it = client_actions.find(action);
    if (it == client_actions.end()) {
        // Not found
        std::cerr << "Operation does not exists" << std::endl;
    }
    else {
        // Correct input - run the mapped function
        func_ptr fp = it->second;
        (this->*fp)();
    }
}
