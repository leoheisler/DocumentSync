#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <iostream>

#include "serverComManager.h" 
#include "commandStatus.h"
#include "fileTransfer.h"
#include "packet.h"
#include "serverFileManager.h"




#define PORT 4000

// CONSTRUCTOR
serverComManager::serverComManager(ClientList* client_list){ this->client_list = client_list;};

// PRIVATE METHODS

void serverComManager::start_communications()
{
	
	Packet starter_packet = Packet::receive_packet(this->client_cmd_socket);
	char* payload = starter_packet.get_payload();
	std::string file_path;
	
	if(payload != nullptr){
		// Extract username and client socket from packet payload
		std::string username = strtok(payload, "\n");
		this->username = username;

		//try to add client to device list
		this->client_list->add_device(username,this->client_cmd_socket);
		this->file_manager.create_server_sync_dir(username);
		this->client_list->display_clients();
	}
	
}


// PUBLIC METHODS

// This is the interface on server that will delegate each method based on commands.
void serverComManager::await_command_packet()
{
	serverFileManager file_manager; 


	while(true){
		// Wait to receive a command packet from client
		Packet command_packet = Packet::receive_packet(this->client_cmd_socket);
		// Determine what to do based on the command packet received
		switch(command_packet.get_seqn()){
			case Command::DOWNLOAD: {
				cout << "recebi o comando de download do user " + this->username << std::endl;
				string file_name = strtok(command_packet.get_payload(), "\n");
				string sync_dir_path = "../src/server/userDirectories/sync_dir_" + this->username;
				string file_path = sync_dir_path + "/" + file_name;
				FileTransfer::send_file(file_path, this->client_fetch_socket);
				break;
			}
			case Command::GET_SYNC_DIR: {
				std::vector<std::string> paths = file_manager.get_sync_dir(this->username);
				for(const std::string& path : paths) {
					FileTransfer::send_file(path, this->client_fetch_socket);
					cout << path << endl;
				}
				break;
			}
		}
	}
}

serverStatus serverComManager::bind_client_sockets(int server_socket, int first_comm_socket){
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(struct sockaddr_in);
	this->client_cmd_socket = first_comm_socket;

    if ((this->client_upload_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
        printf("ERROR on accept upload socket\n");
        return serverStatus::FAILED_TO_ACCEPT_UPLOAD_SOCKET; // Retorna o erro apropriado
    }

    if ((this->client_fetch_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
        printf("ERROR on accept fetch socket\n");
        return serverStatus::FAILED_TO_ACCEPT_FETCH_SOCKET; // Retorna o erro apropriado
    }
	start_communications();	 
	return serverStatus::OK;
		
}
