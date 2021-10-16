#include "ConsoleApp.h"
#include <cassert>

#define NOT_IMPLEMENTED {std::cout << __FUNCTION__ << " not implemented" << std::endl;}

void ConsoleApp::register_client()
{
    if (is_registered())
    {
        std::cout << "User already loaded" << std::endl;
        return;
    }

    ServerRequestHeader request_header{};
    ServerResponseHeader response_header{};
    std::vector<uint8_t> c_payload;

    RegistrationPayload r_payload;
    request_header.version = CLIENT_VERSION;
    request_header.code = static_cast<uint16_t>(ServerRequestCodes::REGISTRATION_CLIENT_REQUEST);
    request_header.payload_size = sizeof(RegistrationPayload);

    // Get username from client
    std::cout << "Please enter registration user name:" << std::endl;
    std::cin.getline(r_payload.name, MAX_REGISTRATION_NAME_LENGTH - 1); // Don't let user to overlap null terminated char

    // Generate public key
    Util::generate_random_key(r_payload.public_key, 160);

    // fill payload into vector
    c_payload.assign((uint8_t*)&r_payload, (uint8_t*)&r_payload + sizeof(RegistrationPayload));

    // Send registration request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, client_id) && response_header.code == static_cast<uint16_t>(ServerResponseCodes::REGISTRATION_SUCCESS))
    {
        std::cout << "Registering with username " << r_payload.name << " ..." << std::endl;

        // Save username and uuid in me.info
        if (create_me_info_file(r_payload.name, &client_id[0], r_payload.public_key))
        {
            std::cout << "Registeration done." << std::endl;
        }
        else
        {
            std::cerr << "Registeration failed - user already exists" << std::endl;
        }
    }
    else
    {
        std::cerr << "Registration Failed: server responded with an error" << std::endl;
    }
}

void ConsoleApp::request_for_client_list()
{
    if (!is_registered()) {
        std::cout << "User is not registered" << std::endl;
        return;
    }

    ServerRequestHeader request_header{};
    ServerResponseHeader response_header{};
    std::vector<uint8_t> c_payload;
    std::vector<uint8_t> s_payload;

    // Initialize request header
    memcpy_s(request_header.client_id, CLIENT_ID_LENGTH, &client_id[0], client_id.size());
    request_header.version = CLIENT_VERSION;
    request_header.code = static_cast<uint16_t>(ServerRequestCodes::CLIENT_LIST_REQUEST);
    request_header.payload_size = 0;

    if (winsock_client.send_request(request_header, c_payload, response_header, s_payload) && response_header.code == static_cast<uint16_t>(ServerResponseCodes::CLIENT_LIST_RESPONSE))
    {
        assert(response_header.payload_size == s_payload.size());
        uint32_t num_of_clients = response_header.payload_size / (CLIENT_ID_LENGTH + MAX_REGISTRATION_NAME_LENGTH);
        std::cout << "There are " << num_of_clients << " in our list:" << std::endl;
        for (uint32_t i = 0; i < num_of_clients; i++)
        {
            uint32_t current_client = i * (CLIENT_ID_LENGTH + MAX_REGISTRATION_NAME_LENGTH);
            // Print client ID
            for (uint32_t client_id_index = 0; client_id_index < CLIENT_ID_LENGTH; client_id_index++)
            {
                std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(s_payload[current_client + client_id_index]);
            }
            std::cout << "\n";
            // Print name
            std::cout << &s_payload[current_client + CLIENT_ID_LENGTH] << std::endl;
        }
    }
    else
    {
        std::cerr << "Request for client list failed: server responded with an error" << std::endl;
    }
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

bool ConsoleApp::create_me_info_file(const std::string& username, const uint8_t* uuid, const uint8_t* public_key) const
{
    // Check if already exists
    if (std::filesystem::exists(ME_INFO_PATH)) {
        return false;
    }

    // Create the file using file stream
    std::ofstream fileStream(ME_INFO_PATH);

    // Write username
    fileStream.write(username.c_str(), username.length());
    fileStream.write("\n", 1);

    // Write UUID
    std::stringstream uuid_stream;
    for (size_t i = 0; i < CLIENT_ID_LENGTH; i++)
    {
        uuid_stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(uuid[i]);
    }
    std::string uuid_as_str(uuid_stream.str());
    fileStream.write(uuid_as_str.c_str(), uuid_as_str.length());
    fileStream.write("\n", 1);

    // Write Public key - TODO should be base64
    std::stringstream public_key_stream;
    for (size_t i = 0; i < PUBLIC_KEY_LENGTH; i++)
    {
        public_key_stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(public_key[i]);
    }
    std::string public_key_as_str(public_key_stream.str());
    fileStream.write(public_key_as_str.c_str(), public_key_as_str.length());

    // close stream
    fileStream.close();
    return true;
}

void ConsoleApp::load_me_info_file()
{
    // First check if me info file exists
    if (!std::filesystem::exists(ME_INFO_PATH)) {
        return;
    }

    std::string ignore_string, uuid_string;
    std::ifstream me_info_file_stream(ME_INFO_PATH);

    // Ignore the user name line
    std::getline(me_info_file_stream, ignore_string);

    // Convert UUID from hex to ASCII
    std::getline(me_info_file_stream, uuid_string);

    for (uint32_t i = 0; i < uuid_string.length(); i += 2) {
        std::string byte = uuid_string.substr(i, 2);
        uint8_t byte_as_ascii = static_cast<uint8_t>(std::strtoul(byte.c_str(), NULL, 16));
        client_id.push_back(byte_as_ascii);
    }
}

bool ConsoleApp::is_registered()
{
    return client_id.size() == CLIENT_ID_LENGTH;
}

std::map<std::string, ConsoleApp::func_ptr> ConsoleApp::create_client_action_map()
{
    std::map<std::string, func_ptr> temp_functions_map = {
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
    return temp_functions_map;
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
    // Try to load me info file
    load_me_info_file();

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
    auto it = client_actions_map.find(action);
    if (it == client_actions_map.end()) {
        // Not found
        std::cerr << "Operation does not exists" << std::endl;
    }
    else {
        // Correct input - run the mapped function
        func_ptr fp = it->second;
        (this->*fp)();
    }
}

ConsoleApp::ConsoleApp() : client_actions_map(create_client_action_map())
{
}
