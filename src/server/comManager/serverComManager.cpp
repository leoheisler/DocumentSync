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

#define PORT 4000

// CONSTRUCTOR
serverComManager::serverComManager(){};

// PRIVATE METHODS
void serverComManager::evaluate_command(Packet command_packet, int socket)
{
	int command_id = command_packet.get_seqn();
	switch(command_id){
		case Command::GET_SYNC_DIR:
			cout << "Received get_sync_dir packet" << endl;
			create_sync_dir(command_packet, socket);
			break;
	}
}

void serverComManager::await_command_packet(int socket)
{
	while(true){
		// Wait to receive a command packet from client
		Packet command_packet = Packet::receive_packet(socket);

		// Determine what to do based on the command packet received
		switch(command_packet.get_type()){
			case Packet::CMD_PACKET:
				evaluate_command(command_packet, socket);
				break;
		}
	}
}

void serverComManager::create_sync_dir(Packet sync_dir_packet, int socket)
{
	char* payload = sync_dir_packet.get_payload();

	if(payload != nullptr){
		// Extract usarnem and client socket from packet payload
		std::string username = strtok(payload, "\n");
		int socket = atoi(strtok(nullptr, "\0"));

		//cout << username << endl << socket << endl;
		clientDeviceList.add_device(username,socket);
		clientDeviceList.display_clients();

	}
}

// PUBLIC METHODS
int serverComManager::connect_server_to_client(int argc, char* argv[])
{
	int sockfd, newsockfd, n;
	//sockfd is the listening socket for accepting new connections.
	//newsockfd is a separate socket used for actual communication with the connected client.
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	
	// SOCKET
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);     
    
	// BIND
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		printf("ERROR on binding");
	
	// LISTEN
	listen(sockfd, 5);
	
	// ACCEPT
	clilen = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) 
		printf("ERROR on accept");
	
	bzero(buffer, 256);
	
	// Receive the file
    // std::string output_file_path ="../src/server/syncDir/test.mp4";
    // FileTransfer::receive_file(output_file_path, newsockfd);
	await_packets(newsockfd);

	close(newsockfd);
	close(sockfd);
	return 0; 
}