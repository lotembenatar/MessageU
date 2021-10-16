#pragma once
#include <cstdint>

#pragma pack(push, 1)

constexpr uint32_t CLIENT_ID_LENGTH = 16;
constexpr uint32_t MAX_REGISTRATION_NAME_LENGTH = 255;
constexpr uint32_t PUBLIC_KEY_LENGTH = 160;

struct ServerRequestHeader
{
	uint8_t client_id[CLIENT_ID_LENGTH];
	uint8_t version;
	uint16_t code;
	uint32_t payload_size;
};

enum class ServerRequestCodes : uint16_t
{
	REGISTRATION_CLIENT_REQUEST = 1000,
	CLIENT_LIST_REQUEST = 1001,
	PUBLIC_KEY_REQUEST = 1002,
	SEND_MESSAGE_TO_CLIENT = 1003,
	WAITING_MESSAGES_REQUEST = 1004,
};

struct RegistrationPayload
{
	char name[MAX_REGISTRATION_NAME_LENGTH] = { 0 };
	uint8_t public_key[PUBLIC_KEY_LENGTH];
};

struct RetrieveClientPublicKeyPayload
{
	uint8_t client_id[CLIENT_ID_LENGTH];
};

struct SendMessageToClientPayloadHeader
{
	uint8_t client_id[CLIENT_ID_LENGTH];
	uint8_t message_type;
	uint32_t content_size;
};

enum class ClientMessageType : uint8_t
{
	SYMMETRIC_KEY_REQUEST = 1,
	SEND_SYMMETRIC_KEY = 2,
	SEND_TEXT_MESSAGE = 3,
	SEND_FILE = 4,
};

struct ServerResponseHeader
{
	uint8_t version;
	uint16_t code;
	uint32_t payload_size;
};

enum class ServerResponseCodes : uint16_t
{
	REGISTRATION_SUCCESS = 2000,
	CLIENT_LIST_RESPONSE = 2001,
	PUBLIC_KEY_RESPONSE = 2002,
	MESSAGE_TO_CLIENT_SENT_TO_SERVER = 2003,
	WAITING_MESSAGES_RESPONSE = 2004,
	GENERAL_FAILURE = 9000
};

#pragma pack(pop)