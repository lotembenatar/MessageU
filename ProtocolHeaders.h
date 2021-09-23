#pragma once
#include <cstdint>

#pragma pack(push, 1)

struct server_request_header
{
	uint8_t client_id[16];
	uint8_t version;
	uint16_t code;
	uint32_t payload_size;
};

enum server_request_codes : uint16_t
{
	REGISTRATION_CLIENT_REQUEST = 1000,
	CLIENT_LIST_REQUEST = 1001,
	PUBLIC_KEY_REQUEST = 1002,
	SEND_MESSAGE_TO_CLIENT = 1003,
	WAITING_MESSAGES_REQUEST = 1004,
};

struct registration_payload
{
	uint8_t name[255];
	uint8_t public_key[160];
};

struct retrieve_client_public_key_payload
{
	uint8_t client_id[16];
};

struct send_message_to_client_payload_header
{
	uint8_t client_id[16];
	uint8_t message_type;
	uint32_t content_size;
	// the payload content should be added after this struct
};

enum client_message_type : uint8_t
{
	SYMMETRIC_KEY_REQUEST = 1,
	SEND_SYMMETRIC_KEY = 2,
	SEND_TEXT_MESSAGE = 3,
	SEND_FILE = 4,
};

struct server_response_header
{
	uint8_t version;
	uint16_t code;
	uint32_t payload_size;
};

#pragma pack(pop)