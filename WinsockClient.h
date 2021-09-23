#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include "ProtocolHeaders.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

constexpr int DEFAULT_BUFLEN = 512;
constexpr const char SERVER_INFO_PATH[] = "server.info";

class WinsockClient
{
private:
	SOCKET connect_socket = INVALID_SOCKET;

	// Read server info functions
	void read_file(const std::string& filepath, std::string& file_content);
	void parse_address_and_port(const std::string& filepath, std::string& servername, std::string& port);

	// Connect the server saved in server.info
	bool connect_server();

	// Shut down the client connection
	bool disconnect_server();

public:
	// Send request to server and return back the response
	bool send_request(const server_request_header& request_header, const std::vector<uint8_t>& client_payload, server_response_header& response_header, std::vector<uint8_t>& server_payload);
};

