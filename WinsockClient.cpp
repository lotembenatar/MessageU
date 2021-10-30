#include "WinsockClient.h"

#ifdef _DEBUG
#define PRINT_ERROR {std::cerr << "Error in " << __FUNCTION__ << " at line " << __LINE__ << std::endl;}
#else
#define PRINT_ERROR
#endif

bool WinsockClient::parse_address_and_port(std::string& servername, std::string& port)
{
    std::string file_content;

    // read file
    if (Util::read_file(SERVER_INFO_PATH, file_content))
    {
        // find the colon seperator index
        size_t colon_index = file_content.find(":");

        // copy the servername
        servername = file_content.substr(0, colon_index);

        // copy port section
        port = file_content.substr(colon_index + 1);

        return true;
    }

    // File not found
    std::cerr << "File " << SERVER_INFO_PATH << " Not found" << std::endl;
    return false;
}

bool WinsockClient::connect_server()
{
    WSADATA wsa_data;
    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints{};
    int iResult = 0;
    std::string servername;
    std::string port;

    // Get server address and port from server.info
    if (!parse_address_and_port(servername, port)) {
        std::cerr << "Was not able to parse server address and port" << std::endl;
        return false;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        return false;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(servername.c_str(), port.c_str(), &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed with error: " << iResult << std::endl;
        WSACleanup();
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connect_socket == INVALID_SOCKET) {
            std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        // Connect to server.
        iResult = connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connect_socket);
            connect_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (connect_socket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return false;
    }

    return true;
}

bool WinsockClient::disconnect_server()
{
    int iResult = 0;

    // shutdown the send half of the connection since no more data will be sent
    iResult = shutdown(connect_socket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "shutdown failed: " << WSAGetLastError() << std::endl;
        closesocket(connect_socket);
        WSACleanup();
        return false;
    }

    return true;
}

bool WinsockClient::send_request(const ServerRequestHeader& request_header, const std::vector<uint8_t>& client_payload, ServerResponseHeader& response_header, std::vector<uint8_t>& server_payload)
{
    int iBytesReceived = 0;
    int iBytesSent = 0;
    server_payload.clear();
    uint8_t recvbuf[DEFAULT_BUFLEN] = { 0 };

    // First connect to server
    if (!connect_server()) return false;

    // Send the request header
    iBytesSent = send(connect_socket, (char*)&request_header, sizeof(request_header), 0);
    if (iBytesSent == SOCKET_ERROR) {
        std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
        closesocket(connect_socket);
        WSACleanup();
        return false;
    }

    // Send the payload - if needed
    if (client_payload.size() > 0)
    {
        iBytesSent = send(connect_socket, (char*)&client_payload[0], static_cast<uint32_t>(client_payload.size()), 0);
        if (iBytesSent == SOCKET_ERROR) {
            std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
            closesocket(connect_socket);
            WSACleanup();
            return false;
        }
    }

    // Shut down the server connection because no more data will be sent
    if (!disconnect_server()) return false;

    // Retrieve the response header
    iBytesReceived = recv(connect_socket, (char*)&response_header, sizeof(response_header), MSG_WAITALL);
    if (iBytesReceived <= 0)
    {
        std::cerr << "recv failed or connection closed" << std::endl;
        closesocket(connect_socket);
        WSACleanup();
        return false;
    }

    // Retrieve the payload
    while (response_header.payload_size > server_payload.size())
    {
        iBytesReceived = recv(connect_socket, (char*)recvbuf, DEFAULT_BUFLEN, 0);
        if (iBytesReceived > 0) {
            server_payload.insert(server_payload.end(), recvbuf, recvbuf + iBytesReceived);
        }
        else if (iBytesReceived <= 0) {
            std::cerr << "recv failed or connection closed" << std::endl;
            closesocket(connect_socket);
            WSACleanup();
            return false;
        }
    }

    // cleanup
    closesocket(connect_socket);
    WSACleanup();

    // Exit success
    return true;
}
