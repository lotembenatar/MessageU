#!/usr/bin/python

import socket
import threading
import struct
import uuid
import random
from enum import Enum

# Constants

SERVER_VERSION = 1
CLIENT_UUID_LENGTH = 16
PUBLIC_KEY_LENGTH = 160
CLIENT_NAME_MAX_LENGTH = 255
REGISTRATION_PAYLOAD_SIZE = 415
SEND_MESSAGE_PAYLOAD_HEADER_SIZE = 21

# Protocol enums

class ClientCodes(Enum):
    REGISTRATION_CLIENT_REQUEST = 1000
    CLIENT_LIST_REQUEST = 1001
    PUBLIC_KEY_REQUEST = 1002
    SEND_MESSAGE_TO_CLIENT = 1003
    WAITING_MESSAGES_REQUEST = 1004

class ServerCodes(Enum):
    REGISTRATION_SUCCESS = 2000
    CLIENT_LIST_RESPONSE = 2001
    PUBLIC_KEY_RESPONSE = 2002
    MESSAGE_TO_CLIENT_SENT_TO_SERVER = 2003
    WAITING_MESSAGES_RESPONSE = 2004
    GENERAL_FAILURE = 9000

class MessageType(Enum):
    SYMMETRIC_KEY_REQUEST = 1
    SEND_SYMMETRIC_KEY = 2
    SEND_TEXT_MESSAGE = 3
    SEND_FILE = 4

class Message:
    def __init__(self, type, sender, content):
        self.type = type
        self.sender = sender
        self.content = content
        self.message_uuid = random.randint(0,0xffffffff)

class ClientStruct:
    def __init__(self, name, public_key) -> None:
        self.name = name
        self.uuid = bytes.fromhex(uuid.uuid4().hex) # Probability grantee us that there is no other user with this UUID
        self.public_key = public_key
        self.waiting_messages = [] # List of Message

    def add_message(self, message_type, sender, message_content):
        message = Message(message_type, sender, message_content)
        self.waiting_messages.append(message)
        return message.message_uuid

    def pull_messages(self):
        messages_copy = self.waiting_messages.copy()
        self.waiting_messages = [] # Delete messages
        return messages_copy

clients = [] # List of ClientStruct

def is_client_uuid_exists(client_uuid):
    for client in clients:
        if client.uuid == client_uuid:
            return True
    return False

def is_client_name_exists(client_name):
    for client in clients:
        if client.name == client_name:
            return True
    return False

class RequestHandler:
    def register_user(self, clientsocket, client_name, client_public_key):
        print("Registering %s ..." % client_name)
        # Check that client_name does not already exists in our server
        if is_client_name_exists(client_name):
            # Name already taken
            server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.GENERAL_FAILURE.value, 0)
            print("Response from server:\nHeader = %s" % server_header)
            # Send back response
            clientsocket.sendall(server_header)
            return
        # Save user
        new_client = ClientStruct(client_name, client_public_key)
        clients.append(new_client)
        # Create header
        server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.REGISTRATION_SUCCESS.value, len(new_client.uuid))
        print("Response from server:\nHeader = %s\nPayload = %s" % (server_header,new_client.uuid))
        # Send back response
        clientsocket.sendall(server_header + new_client.uuid)
    
    def client_list_request(self, clientsocket):
        # Send header
        server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.CLIENT_LIST_RESPONSE.value, len(clients) * (CLIENT_UUID_LENGTH + CLIENT_NAME_MAX_LENGTH))
        clientsocket.sendall(server_header)
        # Send each saved client
        for client in clients:
            client_null_terminated_str_name = client.name.encode() + b'\0'*(CLIENT_NAME_MAX_LENGTH - len(client.name.encode()))
            clientsocket.sendall(client.uuid + client_null_terminated_str_name)
    
    def public_key_request(self, clientsocket, client_uuid):
        for client in clients:
            if client_uuid == client.uuid:
                # Send back public key
                server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.PUBLIC_KEY_RESPONSE.value, len(client.uuid) + len(client.public_key))
                server_payload = client.uuid + client.public_key
                clientsocket.sendall(server_header + server_payload)
                return
        
        # Failure - client not found
        print("Client not found")
        server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.GENERAL_FAILURE.value, 0)
        clientsocket.sendall(server_header)

    def text_message_request(self, clientsocket, sender_client, dest_client, message_type, message_content):
        for client in clients:
            if dest_client == client.uuid:
                # Add message to client list
                message_uuid = client.add_message(message_type, sender_client, message_content)
                # Send back success
                server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.MESSAGE_TO_CLIENT_SENT_TO_SERVER.value, len(dest_client) + 4)
                server_payload = struct.pack('<%ds I' % CLIENT_UUID_LENGTH, dest_client, message_uuid)
                clientsocket.sendall(server_header + server_payload)
                return

    def awaiting_messages_request(self, clientsocket, client_uuid):
        server_payload = b""
        for client in clients:
            if client_uuid == client.uuid:
                for message in client.pull_messages():
                    server_payload += struct.pack('<%ds I B I' % CLIENT_UUID_LENGTH, message.sender, message.message_uuid, message.type, len(message.content)) + message.content
        server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.WAITING_MESSAGES_RESPONSE.value, len(server_payload))
        clientsocket.sendall(server_header + server_payload)

class Server:
    def __init__(self):
        self.DEFAULT_BUFLEN = 512
        self.host = "127.0.0.1"
        # Read port from file
        try:
            self.port = open("port.info", "r").read().strip()
        except:
            print("Error: Unable to parse server port")
        self.request_handler = RequestHandler()
        
        # create an INET, STREAMing socket
        self.serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # bind the socket
        self.serversocket.bind((self.host, int(self.port)))

        # become a server socket
        self.serversocket.listen()

    def start(self):
        """This function starts accepting client connections"""
        while True:
            # accept connections from outside
            (clientsocket, address) = self.serversocket.accept()
            print("client connected:",address)
            # now do something with the clientsocket
            threading.Thread(target=self.handle_client, args=(clientsocket,)).start()

    def handle_client(self, clientsocket):
        """Client entry point"""
        # Receive header from client
        client_id, client_version, client_code, client_payload_size = struct.unpack('<%ds B H I' % CLIENT_UUID_LENGTH, clientsocket.recv(23))
        # Print header for monitoring
        print("Client ID = %s\nClient Version = %d\nClient Code = %d\nClient Payload Size = %d" % (client_id, client_version, client_code, client_payload_size))
        # Handle request
        if client_code == ClientCodes.REGISTRATION_CLIENT_REQUEST.value:
            # Get payload from user
            if client_payload_size != REGISTRATION_PAYLOAD_SIZE:
                print("Error: Incorrect payload size, Got %d and expected %d" % (client_payload_size, REGISTRATION_PAYLOAD_SIZE))
                return
            try:
                client_name, client_public_key = struct.unpack('<%ds %ds' % (CLIENT_NAME_MAX_LENGTH, PUBLIC_KEY_LENGTH), clientsocket.recv(REGISTRATION_PAYLOAD_SIZE))
            except:
                print("Error: Could not get client payload")
                return
            client_name = client_name.rstrip(b'\0').decode() # Remove trailing zeros
            # Print for monitoring
            print("Client name = %s\nClient public key = %s" % (client_name, client_public_key))
            self.request_handler.register_user(clientsocket, client_name, client_public_key)

        elif client_code == ClientCodes.CLIENT_LIST_REQUEST.value:
            if is_client_uuid_exists(client_id):
                self.request_handler.client_list_request(clientsocket)
            else:
                # Cannot serve unregistered client
                server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.GENERAL_FAILURE.value, 0)
                print("Response from server:\nHeader = %s" % server_header)
                # Send back response
                clientsocket.sendall(server_header)

        elif client_code == ClientCodes.PUBLIC_KEY_REQUEST.value:
            if is_client_uuid_exists(client_id):
                # Get payload from user
                try:
                    client_uuid = struct.unpack('<%ds' % CLIENT_UUID_LENGTH, clientsocket.recv(CLIENT_UUID_LENGTH))
                except:
                    print("Error: Could not get client payload")
                    return
                client_uuid = client_uuid[0] # Convert from tuple
                # Print for monitoring
                print("Client uuid = %s" % (client_uuid))
                self.request_handler.public_key_request(clientsocket, client_uuid)
            else:
                # Cannot serve unregistered client
                server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.GENERAL_FAILURE.value, 0)
                print("Response from server:\nHeader = %s" % server_header)
                # Send back response
                clientsocket.sendall(server_header)

        elif client_code == ClientCodes.SEND_MESSAGE_TO_CLIENT.value:
            if is_client_uuid_exists(client_id):
                if client_payload_size < SEND_MESSAGE_PAYLOAD_HEADER_SIZE:
                    print("Error: Payload header is too small, Got %d and expected header is %d" % (client_payload_size, SEND_MESSAGE_PAYLOAD_HEADER_SIZE))
                    return
                try:
                    dest_client, message_type, message_size = struct.unpack('<%ds B I' % CLIENT_UUID_LENGTH, clientsocket.recv(SEND_MESSAGE_PAYLOAD_HEADER_SIZE))
                    message_content = b''
                    while len(message_content) < message_size:
                        message_content += clientsocket.recv(self.DEFAULT_BUFLEN)
                except:
                    print("Error: Could not get client payload")
                    return
                print("Client ID = %s\nDestination client ID = %s\nMessage type = %d\nMessage content = %s" % (client_id, dest_client, message_type, message_content))
                self.request_handler.text_message_request(clientsocket, client_id, dest_client, message_type, message_content)
            else:
                # Cannot serve unregistered client
                server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.GENERAL_FAILURE.value, 0)
                print("Response from server:\nHeader = %s" % server_header)
                # Send back response
                clientsocket.sendall(server_header)

        elif client_code == ClientCodes.WAITING_MESSAGES_REQUEST.value:
            if is_client_uuid_exists(client_id):
                self.request_handler.awaiting_messages_request(clientsocket, client_id)
            else:
                # Cannot serve unregistered client
                server_header = struct.pack('<B H I', SERVER_VERSION, ServerCodes.GENERAL_FAILURE.value, 0)
                print("Response from server:\nHeader = %s" % server_header)
                # Send back response
                clientsocket.sendall(server_header)

        else:
            print("Unsupported request from client", client_code)
        # Close connection
        clientsocket.close()

def main():
    server = Server()
    print("Server starting on port %s ..." % server.port)
    server.start()

if __name__ == "__main__":
	main()