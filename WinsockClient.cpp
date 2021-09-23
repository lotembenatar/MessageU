#include "WinsockClient.h"

#ifdef _DEBUG
#define PRINT_ERROR {std::cerr << "Error in " << __FUNCTION__ << " at line " << __LINE__ << std::endl;}
#else
#define PRINT_ERROR
#endif

void WinsockClient::read_file(const std::string& filepath, std::string& file_content)
{
    std::ifstream file_stream(filepath);
    std::stringstream read_buffer;
    read_buffer << file_stream.rdbuf();

    // copy to returned string
    file_content = read_buffer.str();

    // close stream
    file_stream.close();
}

void WinsockClient::parse_address_and_port(const std::string& filepath, std::string& servername, std::string& port)
{
    std::string file_content;

    // read file
    read_file(filepath, file_content);

    // find the colon seperator index
    size_t colon_index = file_content.find(":");

    // copy the servername
    servername = file_content.substr(0, colon_index);

    // copy port section
    port = file_content.substr(colon_index + 1);
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
    parse_address_and_port(SERVER_INFO_PATH, servername, port);

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(servername.c_str(), port.c_str(), &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connect_socket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
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
        printf("Unable to connect to server!\n");
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
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(connect_socket);
        WSACleanup();
        return false;
    }

    return true;
}

bool WinsockClient::send_request(const server_request_header& request_header, const std::vector<uint8_t>& client_payload, server_response_header& response_header, std::vector<uint8_t>& server_payload)
{
    int iBytesReceived = 0;
    int iBytesSent = 0;
    server_payload.clear();
    uint8_t recvbuf[DEFAULT_BUFLEN] = { 0 };

    // First connect to server
    if (!connect_server())
    {
        std::cerr << "Unable to connect to server" << std::endl;
        return false;
    }

    // Send the request header
    iBytesSent = send(connect_socket, (char*)&request_header, sizeof(request_header), 0);
    if (iBytesSent == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connect_socket);
        WSACleanup();
        return false;
    }

    // Send the payload - if needed
    if (client_payload.size() > 0)
    {
        iBytesSent = send(connect_socket, (char*)&client_payload[0], client_payload.size(), 0);
        if (iBytesSent == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connect_socket);
            WSACleanup();
            return false;
        }
    }

    // Shut down the server connection because no more data will be sent
    if (!disconnect_server())
    {
        std::cerr << "Unable to disconnect from server" << std::endl;
        return false;
    }

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
