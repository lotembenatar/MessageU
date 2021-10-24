#!/usr/bin/python

import socket
import threading
import struct
import uuid
import random
from enum import Enum

server_version = 1
clients = [] # List of ClientStruct

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

class Message:
    def __init__(self, message):
        self.message = message # really??

class ClientStruct:
    def __init__(self, name, public_key) -> None:
        self.name = name
        self.uuid = bytes.fromhex(uuid.uuid4().hex)
        self.public_key = public_key
        self.waiting_messages = [] # List of Message

    def add_message(self, message):
        pass

    def pull_messages(self):
        pass

class RequestHandler:
    def register_user(self, clientsocket):
        # Get payload from user
        try:
            client_name, client_public_key = struct.unpack('<255s 160s', clientsocket.recv(415))
        except:
            print("Error: Could not get client payload")
            return
        client_name = client_name.rstrip(b'\0').decode() # Remove trailing zeros
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
    
    def public_key_request(self, clientsocket):
        # Get payload from user
        try:
            client_uuid = struct.unpack('<16s', clientsocket.recv(16))
        except:
            print("Error: Could not get client payload")
            return
        
        for client in clients:
            if client_uuid[0] == client.uuid:
                # Send back public key
                server_header = struct.pack('<B H I', server_version, ServerCodes.PUBLIC_KEY_RESPONSE.value, len(client.uuid) + len(client.public_key))
                server_payload = client.uuid + client.public_key
                clientsocket.sendall(server_header + server_payload)
                return
        
        # Failure - client not found
        print("Client not found")
        server_header = struct.pack('<B H I', server_version, ServerCodes.GENERAL_FAILURE.value, 0)
        clientsocket.sendall(server_header)

    def text_message_request(self, clientsocket):
        print("Text message request not implemented")

    def awaiting_messages_request(self, clientsocket):
        print("Awaiting messages request not implemented")

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
            # register user
            self.request_handler.register_user(clientsocket)
        elif client_code == ClientCodes.CLIENT_LIST_REQUEST.value:
            self.request_handler.client_list_request(clientsocket)
        elif client_code == ClientCodes.PUBLIC_KEY_REQUEST.value:
            self.request_handler.public_key_request(clientsocket)
        elif client_code == ClientCodes.SEND_MESSAGE_TO_CLIENT.value:
            self.request_handler.text_message_request(clientsocket)
        elif client_code == ClientCodes.WAITING_MESSAGES_REQUEST.value:
            self.request_handler.awaiting_messages_request(clientsocket)
        else:
            print("Unsupported request from client", client_code)
        # Close connection
        clientsocket.close()

server = Server()
print("Server starting on port %s ..." % server.port)
server.start()
