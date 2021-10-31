#include "ConsoleApp.h"
#include <cassert>

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

    // Create an RSA decryptor. this is done here to generate a new private/public key pair
    RSAPrivateWrapper rsapriv;

    // Generate public key
    rsapriv.getPublicKey(r_payload.public_key, RSAPublicWrapper::KEYSIZE);

    // Save the private key base64
    base64_private_key = Base64Wrapper::encode(rsapriv.getPrivateKey());

    // Get username from client
    std::cout << "Please enter registration user name:" << std::endl;
    std::cin.getline(r_payload.name, MAX_REGISTRATION_NAME_LENGTH - 1); // Don't let user to overlap null terminated char

    // fill payload into vector
    c_payload.assign((uint8_t*)&r_payload, (uint8_t*)&r_payload + sizeof(RegistrationPayload));

    // Send registration request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, client_id) && response_header.code == ServerResponseCodes::REGISTRATION_SUCCESS)
    {
        std::cout << "Registering with username " << r_payload.name << " ..." << std::endl;

        // Save username and uuid in me.info
        if (create_me_info_file(r_payload.name, &client_id[0]))
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
            // calculate client index in the returned payload
            uint32_t current_client_index = i * (CLIENT_ID_LENGTH + MAX_REGISTRATION_NAME_LENGTH);

            // Create client
            Client current_client;
            current_client.name = std::string((char*)(&s_payload[current_client_index + CLIENT_ID_LENGTH]));
            current_client.uuid = std::vector<uint8_t>(s_payload.begin() + current_client_index, s_payload.begin() + current_client_index + CLIENT_ID_LENGTH);

            // Check if client already exists in our map
            auto it = username_to_client_map.find(current_client.name);
            if (it == username_to_client_map.end()) {
                // Not found - add it
                username_to_client_map[current_client.name] = current_client;
            }

            // Print name
            std::cout << current_client.name << std::endl;
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
    std::vector<uint8_t> s_payload;
    std::string dest_username;

    // Initialize request header
    memcpy_s(request_header.client_id, CLIENT_ID_LENGTH, &client_id[0], client_id.size());
    request_header.version = CLIENT_VERSION;
    request_header.code = ServerRequestCodes::PUBLIC_KEY_REQUEST;
    request_header.payload_size = CLIENT_ID_LENGTH;

    // Get UUID from user
    std::cout << "Enter user name for request:" << std::endl;
    std::getline(std::cin, dest_username);

    // Figure out the UUID of the destination user by its name
    auto it = username_to_client_map.find(dest_username);
    if (it == username_to_client_map.end()) {
        // Not found
        std::cerr << "No user with such name (You may need to update your user list)" << std::endl;
        return;
    }

    // Send request to server
    if (winsock_client.send_request(request_header, it->second.uuid, response_header, s_payload) && response_header.code == ServerResponseCodes::PUBLIC_KEY_RESPONSE)
    {
        assert(response_header.payload_size == s_payload.size());

        // Save public key for this client
        it->second.public_key.assign(s_payload.begin() + CLIENT_ID_LENGTH, s_payload.begin() + CLIENT_ID_LENGTH + RSAPublicWrapper::KEYSIZE);
        
        // Print client public key to console
        for (uint32_t i = 0; i < RSAPublicWrapper::KEYSIZE; i++)
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
            std::string client_name;
            // Linear search on binary tree to find client name
            for (const auto& pair : username_to_client_map) {
                std::vector<uint8_t> uuid_vector;
                uuid_vector.assign(message_header->client_id, message_header->client_id + CLIENT_ID_LENGTH);
                if (pair.second.uuid == uuid_vector)
                {
                    client_name = pair.first;
                    break;
                }
            }
            if (client_name.size() > 0)
            {
                std::cout << "From: " << client_name << "\nContent:\n";
                if (message_header->message_type == ClientMessageType::SYMMETRIC_KEY_REQUEST)
                {
                    std::cout << "Request for symmetric key";
                }
                else if (message_header->message_type == ClientMessageType::SEND_SYMMETRIC_KEY)
                {
                    std::string ciphertext;
                    for (uint32_t i = 0; i < message_header->message_size; i++)
                    {
                        ciphertext += s_payload[s_payload_index + sizeof(WaitingMessageResponseHeader) + i];
                    }

                    // Decrypt symmetric key with private key
                    RSAPrivateWrapper rsapriv(Base64Wrapper::decode(base64_private_key));
                    std::string plaintext_key = rsapriv.decrypt(ciphertext);

                    // Save symmetric key for the user
                    username_to_client_map[client_name].session_key.assign(plaintext_key.begin(), plaintext_key.end());

                    // Print to user that key have been recieved
                    std::cout << "symmetric key recieved";
                }
                else if (message_header->message_type == ClientMessageType::SEND_TEXT_MESSAGE)
                {
                    std::string ciphertext;
                    std::vector<uint8_t>& session_key = username_to_client_map[client_name].session_key;
                    // Check that a session key exists between these two clients
                    if (session_key.empty())
                    {
                        std::cerr << "can’t decrypt message";
                    }
                    else
                    {
                        for (uint32_t i = 0; i < message_header->message_size; i++)
                        {
                            ciphertext += s_payload[s_payload_index + sizeof(WaitingMessageResponseHeader) + i];
                        }
                        // Decrypt cipher to plaintext
                        AESWrapper aes(&session_key[0], session_key.size());
                        std::string plaintext = aes.decrypt(ciphertext.c_str(), static_cast<uint32_t>(ciphertext.length()));
                        std::cout << plaintext;
                    }
                }
                else if (message_header->message_type == ClientMessageType::SEND_FILE)
                {
                    std::string temp_file_path = std::filesystem::temp_directory_path().generic_string() + std::to_string(message_header->message_id);
                    std::string ciphertext;
                    std::vector<uint8_t>& session_key = username_to_client_map[client_name].session_key;
                    // Check that a session key exists between these two clients
                    if (session_key.empty())
                    {
                        std::cerr << "can’t decrypt message";
                    }
                    else
                    {
                        for (uint32_t i = 0; i < message_header->message_size; i++)
                        {
                            ciphertext += s_payload[s_payload_index + sizeof(WaitingMessageResponseHeader) + i];
                        }
                        // Decrypt cipher to plaintext
                        AESWrapper aes(&session_key[0], session_key.size());
                        std::string plaintext = aes.decrypt(ciphertext.c_str(), static_cast<uint32_t>(ciphertext.length()));

                        // Store file
                        std::ofstream fileStream(temp_file_path);
                        fileStream.write(plaintext.c_str(), plaintext.size());
                        fileStream.close();

                        // Print file path
                        std::cout << temp_file_path;
                    }
                }
                else
                {
                    // Error
                    std::cerr << "Error: unknown message type: " << static_cast<uint32_t>(message_header->message_type);
                }
                std::cout << "\n----<EOM>-----" << std::endl;
            }
            else
            {
                // Unknown client
                std::cerr << "Message from unknown user (Please update client list)" << std::endl;
            }
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
    send_message_to_client(ClientMessageType::SEND_TEXT_MESSAGE);
}

void ConsoleApp::send_request_for_symmetric_key()
{
    send_message_to_client(ClientMessageType::SYMMETRIC_KEY_REQUEST);
}

void ConsoleApp::send_symmetric_key()
{
    send_message_to_client(ClientMessageType::SEND_SYMMETRIC_KEY);
}

void ConsoleApp::send_file()
{
    send_message_to_client(ClientMessageType::SEND_FILE);
}

void ConsoleApp::exit_client()
{
    std::cout << "Bye bye!" << std::endl;
    exit(0);
}

void ConsoleApp::send_message_to_client(ClientMessageType message_type)
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
    std::string ciphertext;

    // Get name of the destination user
    std::cout << "Enter destination user name:" << std::endl;
    std::getline(std::cin, dest_username);

    // Find destination user by its name
    auto it = username_to_client_map.find(dest_username);
    if (it == username_to_client_map.end()) {
        // Not found
        std::cerr << "No user with such name (You may need to update your user list)" << std::endl;
        return;
    }

    // Message specific code
    if (message_type == ClientMessageType::SEND_SYMMETRIC_KEY)
    {
        // Check that public key was recieved before for this user
        if (it->second.public_key.size() == 0) {
            std::cerr << "Does not have a public key for this user" << std::endl;
            return;
        }

        // Generate symmetric key and save it in our clients map
        unsigned char key[AESWrapper::DEFAULT_KEYLENGTH];
        AESWrapper::GenerateKey(key, AESWrapper::DEFAULT_KEYLENGTH);
        it->second.session_key.assign(key, key + AESWrapper::DEFAULT_KEYLENGTH);

        // Encrypt symmetric key with destination client public key
        RSAPublicWrapper rsapub((const char*)&it->second.public_key[0], RSAPublicWrapper::KEYSIZE);
        ciphertext = rsapub.encrypt((const char*)key, AESWrapper::DEFAULT_KEYLENGTH);
    }
    else if (message_type == ClientMessageType::SEND_TEXT_MESSAGE)
    {
        // Check that session key was recieved before for this user
        if (it->second.session_key.size() == 0) {
            std::cerr << "Does not have a symmetric key for this user" << std::endl;
            return;
        }

        // Get message from user
        std::cout << "Type your message:" << std::endl;
        std::string message;
        std::getline(std::cin, message);

        if (message.size() > ULONG_MAX) {
            std::cerr << "Message is too big" << std::endl;
            return;
        }

        // Encrypt message with symmetric key
        AESWrapper aes(&it->second.session_key[0], it->second.session_key.size());
        ciphertext = aes.encrypt(message.c_str(), static_cast<uint32_t>(message.length()));
    }
    else if (message_type == ClientMessageType::SEND_FILE)
    {
        // Check that session key was recieved before for this user
        if (it->second.session_key.size() == 0) {
            std::cerr << "Does not have a symmetric key for this user" << std::endl;
            return;
        }

        // Get message from user
        std::cout << "Enter file path:" << std::endl;
        std::string file_path, file_content;
        std::getline(std::cin, file_path);

        if (!Util::read_file(file_path, file_content)) {
            std::cerr << "file not found" << std::endl;
            return;
        }

        // Encrypt message with symmetric key
        AESWrapper aes(&it->second.session_key[0], it->second.session_key.size());
        ciphertext = aes.encrypt(file_content.c_str(), static_cast<uint32_t>(file_content.length()));
    }

    // Assign payload header members
    memcpy_s(payload_header.client_id, CLIENT_ID_LENGTH, &it->second.uuid[0], it->second.uuid.size());
    payload_header.message_type = message_type;
    payload_header.content_size = ciphertext.size();

    // Assign client payload vector
    c_payload.insert(c_payload.begin(), (uint8_t*)&payload_header, (uint8_t*)&payload_header + sizeof(SendMessageToClientPayloadHeader));
    c_payload.insert(c_payload.end(), ciphertext.begin(), ciphertext.end());

    // Initialize request header
    memcpy_s(request_header.client_id, CLIENT_ID_LENGTH, &client_id[0], client_id.size());
    request_header.version = CLIENT_VERSION;
    request_header.code = ServerRequestCodes::SEND_MESSAGE_TO_CLIENT;
    request_header.payload_size = static_cast<uint32_t>(c_payload.size());

    // Send request to server
    if (winsock_client.send_request(request_header, c_payload, response_header, s_payload) && response_header.code == ServerResponseCodes::MESSAGE_TO_CLIENT_SENT_TO_SERVER)
    {
        std::cout << "Message sent to server" << std::endl;
    }
    else
    {
        std::cerr << "Send text message failed: server responded with an error" << std::endl;
    }
}

bool ConsoleApp::create_me_info_file(const std::string& username, const uint8_t* uuid) const
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

    // Write private key
    fileStream.write(base64_private_key.c_str(), base64_private_key.length());

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

    std::string temp_string, uuid_string;
    std::ifstream me_info_file_stream(ME_INFO_PATH);

    // Ignore the user name in the first line
    std::getline(me_info_file_stream, temp_string);

    // UUID should be in the second line of the file
    std::getline(me_info_file_stream, uuid_string);

    // Convert UUID from hex to ASCII
    Util::convert_hex_str_to_bytes(uuid_string, client_id);

    // Extract private key from rest of the file
    while(std::getline(me_info_file_stream, temp_string))
        base64_private_key += temp_string;

    // Close stream
    me_info_file_stream.close();
}

bool ConsoleApp::is_registered()
{
    return client_id.size() == CLIENT_ID_LENGTH && !base64_private_key.empty();
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
    std::cout << "?\n";
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
