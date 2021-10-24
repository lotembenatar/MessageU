#!/usr/bin/python

import socket
import threading
import struct
import uuid
import random
from enum import Enum

server_version = 1
clients = [] # List of ClientStruct

REGISTRATION_PAYLOAD_SIZE = 415
SEND_MESSAGE_PAYLOAD_HEADER_SIZE = 21

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
        self.uuid = bytes.fromhex(uuid.uuid4().hex)
        self.public_key = public_key
        self.waiting_messages = [] # List of Message

    def add_message(self, message_type, sender, message_content):
        message = Message(message_type, sender, message_content)
        self.waiting_messages.append(message)
        return message.message_uuid

    def pull_messages(self):
        messages_copy = self.waiting_messages.copy()
        self.waiting_messages = []
        return messages_copy

class RequestHandler:
    def register_user(self, clientsocket, client_name, client_public_key):
        print("Registering %s ..." % client_name)
        # Save user
        new_client = ClientStruct(client_name, client_public_key)
        clients.append(new_client)
        # Create header
        server_header = struct.pack('<B H I', server_version, ServerCodes.REGISTRATION_SUCCESS.value, len(new_client.uuid))
        print("Response from server:\nHeader = %s\nPayload = %s" % (server_header,new_client.uuid))
        # Send back response
        clientsocket.sendall(server_header + new_client.uuid)
    
    def client_list_request(self, clientsocket):
        # Send header
        server_header = struct.pack('<B H I', server_version, ServerCodes.CLIENT_LIST_RESPONSE.value, len(clients) * (16 + 255))
        clientsocket.sendall(server_header)
        # Send each saved client
        for client in clients:
            client_null_terminated_str_name = client.name.encode() + b'\0'*(255 - len(client.name.encode()))
            clientsocket.sendall(client.uuid + client_null_terminated_str_name)
    
    def public_key_request(self, clientsocket, client_uuid):
        for client in clients:
            if client_uuid == client.uuid:
                # Send back public key
                server_header = struct.pack('<B H I', server_version, ServerCodes.PUBLIC_KEY_RESPONSE.value, len(client.uuid) + len(client.public_key))
                server_payload = client.uuid + client.public_key
                clientsocket.sendall(server_header + server_payload)
                return
        
        # Failure - client not found
        print("Client not found")
        server_header = struct.pack('<B H I', server_version, ServerCodes.GENERAL_FAILURE.value, 0)
        clientsocket.sendall(server_header)

    def text_message_request(self, clientsocket, sender_client, dest_client, message_type, message_content):
        for client in clients:
            if dest_client == client.uuid:
                # Add message to client list
                message_uuid = client.add_message(message_type, sender_client, message_content)
                # Send back success
                server_header = struct.pack('<B H I', server_version, ServerCodes.MESSAGE_TO_CLIENT_SENT_TO_SERVER.value, len(dest_client) + 4)
                server_payload = struct.pack('<16s I', dest_client, message_uuid)
                clientsocket.sendall(server_header + server_payload)
                return

    def awaiting_messages_request(self, clientsocket, client_uuid):
        server_payload = b""
        for client in clients:
            if client_uuid == client.uuid:
                for message in client.pull_messages():
                    server_payload += struct.pack('<16s I B I', message.sender, message.message_uuid, message.type, len(message.content)) + message.content
        server_header = struct.pack('<B H I', server_version, ServerCodes.WAITING_MESSAGES_RESPONSE.value, len(server_payload))
        clientsocket.sendall(server_header + server_payload)

class Server:
    def __init__(self):
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
        client_id, client_version, client_code, client_payload_size = struct.unpack('<16s B H I', clientsocket.recv(23))
        # Print header for monitoring
        print("Client ID = %s\nClient Version = %d\nClient Code = %d\nClient Payload Size = %d" % (client_id, client_version, client_code, client_payload_size))
        # Handle request
        if client_code == ClientCodes.REGISTRATION_CLIENT_REQUEST.value:
            # Get payload from user
            if client_payload_size != REGISTRATION_PAYLOAD_SIZE:
                print("Error: Incorrect payload size, Got %d and expected %d" % (client_payload_size, REGISTRATION_PAYLOAD_SIZE))
                return
            try:
                client_name, client_public_key = struct.unpack('<255s 160s', clientsocket.recv(REGISTRATION_PAYLOAD_SIZE))
            except:
                print("Error: Could not get client payload")
                return
            client_name = client_name.rstrip(b'\0').decode() # Remove trailing zeros
            self.request_handler.register_user(clientsocket, client_name, client_public_key)

        elif client_code == ClientCodes.CLIENT_LIST_REQUEST.value:
            self.request_handler.client_list_request(clientsocket)

        elif client_code == ClientCodes.PUBLIC_KEY_REQUEST.value:
            # Get payload from user
            try:
                client_uuid = struct.unpack('<16s', clientsocket.recv(16))
            except:
                print("Error: Could not get client payload")
                return
            client_uuid = client_uuid[0] # Convert from tuple
            self.request_handler.public_key_request(clientsocket, client_uuid)

        elif client_code == ClientCodes.SEND_MESSAGE_TO_CLIENT.value:
            if client_payload_size < SEND_MESSAGE_PAYLOAD_HEADER_SIZE:
                print("Error: Payload header is too small, Got %d and expected header is %d" % (client_payload_size, SEND_MESSAGE_PAYLOAD_HEADER_SIZE))
                return
            try:
                dest_client, message_type, message_size = struct.unpack('<16s B I', clientsocket.recv(SEND_MESSAGE_PAYLOAD_HEADER_SIZE))
                message_content = clientsocket.recv(message_size)
            except:
                print("Error: Could not get client payload")
                return
            self.request_handler.text_message_request(clientsocket, client_id, dest_client, message_type, message_content)

        elif client_code == ClientCodes.WAITING_MESSAGES_REQUEST.value:
            self.request_handler.awaiting_messages_request(clientsocket, client_id)

        else:
            print("Unsupported request from client", client_code)
        # Close connection
        clientsocket.close()

server = Server()
print("Server starting on port %s ..." % server.port)
server.start()
