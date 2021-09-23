#include "UserActions.h"

#define NOT_IMPLEMENTED {std::cout << __FUNCTION__ << " not implemented" << std::endl;}

// Initialize const client actions
const std::map<std::string, func_ptr> UserActions::client_actions = UserActions::create_client_action_map();

void UserActions::register_client()
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

void UserActions::request_for_client_list()
{
    NOT_IMPLEMENTED;
}

void UserActions::request_for_public_key()
{
    NOT_IMPLEMENTED;
}

void UserActions::request_for_waiting_messages()
{
    NOT_IMPLEMENTED;
}

void UserActions::send_text_message()
{
    NOT_IMPLEMENTED;
}

void UserActions::send_request_for_symmetric_key()
{
    NOT_IMPLEMENTED;
}

void UserActions::send_symmetric_key()
{
    NOT_IMPLEMENTED;
}

void UserActions::send_file()
{
    NOT_IMPLEMENTED;
}

void UserActions::exit_client()
{
    std::cout << "Bye bye!" << std::endl;
    exit(0);
}

std::map<std::string, func_ptr> UserActions::create_client_action_map()
{
    std::map<std::string, func_ptr> client_actions = {
       {"10" , &register_client},
       {"20" , &request_for_client_list},
       {"30" , &request_for_public_key},
       {"40" , &request_for_waiting_messages},
       {"50" , &send_text_message},
       {"51" , &send_request_for_symmetric_key},
       {"52" , &send_symmetric_key},
       {"53" , &send_file},
       {"0" , &exit_client},
    };
    return client_actions;
}

// TODO - add public key
bool UserActions::create_me_info_file(const std::string& username, uint64_t uuid)
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

void UserActions::display_usage()
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
    std::cout << std::endl; // flush buffer
}

void UserActions::get_action_from_user()
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
        it->second();
    }
}
