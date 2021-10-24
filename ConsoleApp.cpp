#include "ConsoleApp.h"
#include <cassert>

#pragma warning (disable:4702)
#define NOT_IMPLEMENTED {std::cout << __FUNCTION__ << " not implemented" << std::endl; return;}
#define TODO {/*TODO*/}

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
    request_header.code = ServerRequestCodes::REGISTRATION_CLIENT_REQUEST;
    request_header.payload_size = sizeof(RegistrationPayload);

    // Get username from client
    std::cout << "Please enter registration user name:" << std::endl;
    std::cin.getline(r_payload.name, MAX_REGISTRATION_NAME_LENGTH - 1); // Don't let user to overlap null terminated char

    // Generate public key
    Util::generate_random_key(r_payload.public_key, 160);

    // fill payload into vector
    c_payload.assign((uint8_t*)&r_payload, (uint8_t*)&r_payload + sizeof(RegistrationPayload));

    // Send registration request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, client_id) && response_header.code == ServerResponseCodes::REGISTRATION_SUCCESS)
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
    request_header.code = ServerRequestCodes::CLIENT_LIST_REQUEST;
    request_header.payload_size = 0;

    if (winsock_client.send_request(request_header, c_payload, response_header, s_payload) && response_header.code == ServerResponseCodes::CLIENT_LIST_RESPONSE)
    {
        assert(response_header.payload_size == s_payload.size());
        uint32_t num_of_clients = response_header.payload_size / (CLIENT_ID_LENGTH + MAX_REGISTRATION_NAME_LENGTH);
        std::cout << "There are " << num_of_clients << " in our list:" << std::endl;
        for (uint32_t i = 0; i < num_of_clients; i++)
        {
            // calculate client index in returned payload
            uint32_t current_client = i * (CLIENT_ID_LENGTH + MAX_REGISTRATION_NAME_LENGTH);
            // extract username string and uuid and save it in a map
            std::string current_username((char*)(&s_payload[current_client + CLIENT_ID_LENGTH]));
            username_to_uuid_map[current_username] = std::vector<uint8_t>(s_payload.begin() + current_client, s_payload.begin() + current_client + CLIENT_ID_LENGTH);
            // Print client ID
            for (const uint8_t byte : username_to_uuid_map[current_username])
            {
                std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(byte);
            }
            std::cout << " ";
            // Print name
            std::cout << current_username << std::endl;
        }
    }
    else
    {
        std::cerr << "Request for client list failed: server responded with an error" << std::endl;
    }
}

void ConsoleApp::request_for_public_key()
{
    if (!is_registered()) {
        std::cout << "User is not registered" << std::endl;
        return;
    }

    ServerRequestHeader request_header{};
    ServerResponseHeader response_header{};
    std::vector<uint8_t> c_payload;
    std::vector<uint8_t> s_payload;
    std::string uuid_from_user;

    // Initialize request header
    memcpy_s(request_header.client_id, CLIENT_ID_LENGTH, &client_id[0], client_id.size());
    request_header.version = CLIENT_VERSION;
    request_header.code = ServerRequestCodes::PUBLIC_KEY_REQUEST;
    request_header.payload_size = CLIENT_ID_LENGTH;

    // Get UUID from user
    std::cout << "Enter UUID for request:" << std::endl;
    std::getline(std::cin, uuid_from_user);

    // Fill client payload with uuid got from user as bytes
    Util::convert_hex_str_to_bytes(uuid_from_user, c_payload);

    // Send request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, s_payload) && response_header.code == ServerResponseCodes::PUBLIC_KEY_RESPONSE)
    {
        assert(response_header.payload_size == s_payload.size());
        
        // Print client ID
        for (uint32_t i = 0; i < PUBLIC_KEY_LENGTH; i++)
        {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(s_payload[CLIENT_ID_LENGTH + i]);
        }

        std::cout << std::endl;
    }
    else
    {
        std::cerr << "Request for public key failed: server responded with an error" << std::endl;
    }
}

void ConsoleApp::request_for_waiting_messages()
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
    request_header.code = ServerRequestCodes::WAITING_MESSAGES_REQUEST;
    request_header.payload_size = 0;

    // Send request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, s_payload) && response_header.code == ServerResponseCodes::WAITING_MESSAGES_RESPONSE)
    {
        assert(response_header.payload_size == s_payload.size());

        uint32_t s_payload_index = 0;
        while (s_payload_index < s_payload.size())
        {
            WaitingMessageResponseHeader* message_header = (WaitingMessageResponseHeader*)&s_payload[s_payload_index];
            std::cout << "From: ";
            // Linear search on binary tree
            for (const auto& pair : username_to_uuid_map) {
                std::vector<uint8_t> uuid_vector;
                uuid_vector.assign(message_header->client_id, message_header->client_id + CLIENT_ID_LENGTH);
                if (pair.second == uuid_vector)
                {
                    std::cout << pair.first;
                    break;
                }
            }
            std::cout << "\nContent:\n";
            switch (message_header->message_type)
            {
            case ClientMessageType::SYMMETRIC_KEY_REQUEST:
                std::cout << "Request for symmetric key";
                break;
            case ClientMessageType::SEND_SYMMETRIC_KEY:
                std::cout << "symmetric key recieved";
                break;
            case ClientMessageType::SEND_TEXT_MESSAGE:
                for (uint32_t i = 0; i < message_header->message_size; i++)
                {
                    std::cout << s_payload[s_payload_index + sizeof(WaitingMessageResponseHeader) + i];
                }
                break;
            case ClientMessageType::SEND_FILE:
                std::cout << "file";
                break;
            default:
                std::cerr << "Error: unknown message type: " << static_cast<uint32_t>(message_header->message_type);
                break;
            }
            std::cout << "\n----<EOM>-----" << std::endl;
            // Increment index to next message
            s_payload_index += sizeof(WaitingMessageResponseHeader) + message_header->message_size;
        }
    }
    else
    {
        std::cerr << "Request for waiting messages failed: server responded with an error" << std::endl;
    }
}

void ConsoleApp::send_text_message()
{
    if (!is_registered()) {
        std::cout << "User is not registered" << std::endl;
        return;
    }

    ServerRequestHeader request_header{};
    ServerResponseHeader response_header{};
    SendMessageToClientPayloadHeader payload_header{};
    std::vector<uint8_t> c_payload;
    std::vector<uint8_t> s_payload;
    std::string dest_username;
    std::string message;

    // Initialize request header
    memcpy_s(request_header.client_id, CLIENT_ID_LENGTH, &client_id[0], client_id.size());
    request_header.version = CLIENT_VERSION;
    request_header.code = ServerRequestCodes::SEND_MESSAGE_TO_CLIENT;

    // Get name of the destination user
    std::cout << "Enter destination user name:" << std::endl;
    std::getline(std::cin, dest_username);

    // Figure out the UUID of the destination user by its name
    auto it = username_to_uuid_map.find(dest_username);
    if (it == username_to_uuid_map.end()) {
        // Not found
        std::cerr << "No user with such name (You may need to update your user list)" << std::endl;
        return;
    }
    else {
        // Found - Copy the destination id
        memcpy_s(payload_header.client_id, CLIENT_ID_LENGTH, &it->second[0], it->second.size());
    }

    // Get message from user
    std::cout << "Type your message:" << std::endl;
    std::getline(std::cin, message);

    if (message.size() > ULONG_MAX) {
        std::cerr << "Message is too big" << std::endl;
        return;
    }

    // Assign payload header members
    payload_header.message_type = ClientMessageType::SEND_TEXT_MESSAGE;
    payload_header.content_size = static_cast<uint32_t>(message.size());

    // Encrypt message with symmetric key - promt error if there is no symmetric key yet
    TODO;
    
    // Insert payload header and message to payload vector
    c_payload.insert(c_payload.begin(), (uint8_t*)&payload_header, (uint8_t*)(&payload_header) + sizeof(SendMessageToClientPayloadHeader));
    c_payload.insert(c_payload.end(), message.begin(), message.end());

    // Initialize payload size
    request_header.payload_size = static_cast<uint32_t>(c_payload.size());

    // Send request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, s_payload) && response_header.code == ServerResponseCodes::MESSAGE_TO_CLIENT_SENT_TO_SERVER)
    {
        assert(response_header.payload_size == s_payload.size());

        std::cout << "Message sent to server" << std::endl;
    }
    else
    {
        std::cerr << "Send text message failed: server responded with an error" << std::endl;
    }
}

void ConsoleApp::send_request_for_symmetric_key()
{
    if (!is_registered()) {
        std::cout << "User is not registered" << std::endl;
        return;
    }

    ServerRequestHeader request_header{};
    ServerResponseHeader response_header{};
    SendMessageToClientPayloadHeader payload_header{};
    std::vector<uint8_t> c_payload;
    std::vector<uint8_t> s_payload;
    std::string dest_username;

    // Initialize request header
    memcpy_s(request_header.client_id, CLIENT_ID_LENGTH, &client_id[0], client_id.size());
    request_header.version = CLIENT_VERSION;
    request_header.code = ServerRequestCodes::SEND_MESSAGE_TO_CLIENT;
    request_header.payload_size = sizeof(SendMessageToClientPayloadHeader);

    // Get name of the destination user
    std::cout << "Enter destination user name:" << std::endl;
    std::getline(std::cin, dest_username);

    // Figure out the UUID of the destination user by its name
    auto it = username_to_uuid_map.find(dest_username);
    if (it == username_to_uuid_map.end()) {
        // Not found
        std::cerr << "No user with such name (You may need to update your user list)" << std::endl;
        return;
    }
    else {
        // Found - Copy the destination id
        memcpy_s(payload_header.client_id, CLIENT_ID_LENGTH, &it->second[0], it->second.size());
    }

    // Assign payload header members
    payload_header.message_type = ClientMessageType::SYMMETRIC_KEY_REQUEST;
    payload_header.content_size = 0;

    // Insert payload header to client payload vector
    c_payload.insert(c_payload.begin(), (uint8_t*)&payload_header, (uint8_t*)(&payload_header + sizeof(SendMessageToClientPayloadHeader)));

    // Send request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, s_payload) && response_header.code == ServerResponseCodes::MESSAGE_TO_CLIENT_SENT_TO_SERVER)
    {
        assert(response_header.payload_size == s_payload.size());

        std::cout << "Message sent to server" << std::endl;
    }
    else
    {
        std::cerr << "Send text message failed: server responded with an error" << std::endl;
    }
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

    // Ignore the user name in the first line
    std::getline(me_info_file_stream, ignore_string);

    // UUID should be in the second line of the file
    std::getline(me_info_file_stream, uuid_string);

    // Convert UUID from hex to ASCII
    Util::convert_hex_str_to_bytes(uuid_string, client_id);
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
