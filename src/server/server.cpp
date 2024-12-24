#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>    
#include <unistd.h>   
#include <sys/socket.h>
#include <netinet/in.h>
#include "fileTransfer.h"
#include <serverComManager.h>
#include <thread>
#include <mutex>


#define PORT 4000
std::mutex connect_hand;

serverStatus bind_server_socket(int* server_socket){
	struct sockaddr_in serv_addr;

	if ((*server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("ERROR opening socket\n");
		return serverStatus::FAILED_TO_CREATE_SOCKET;
	}
        
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);     
	if (bind(*server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		cout << ("ERROR on binding");
		return serverStatus::FAILED_TO_BIND_SOCKET;
	} 

	return serverStatus::OK;
}

serverStatus setup_backup_server_socket(int port, hostent* server, int* server_socket){
	struct sockaddr_in serv_addr;

	if ((*server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("ERROR opening socket\n");
		return serverStatus::FAILED_TO_CREATE_SOCKET;
	}

	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

    if (connect(*server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        cout << "ERROR connecting cmd socket\n";
    else
        cout <<"backup server socket connected\n";

	return serverStatus::OK;
}

void connection_handler(int server_socket, int first_contact_socket,ClientList* client_device_list, ServerList* backup_server_list){
	// Create a server comm manager for the connection
	serverComManager server_comm(client_device_list, backup_server_list);

	// Send a handshake to client
	Packet handshake_packet(first_contact_socket);
	handshake_packet.send_packet(first_contact_socket);

	Packet ack_packet = Packet::receive_packet(first_contact_socket);
	if(ack_packet.get_seqn()){
		// Client is an user device
		server_comm.bind_client_sockets(server_socket, first_contact_socket);
	}else{
		// Client is a backup server
		server_comm.add_backup_server(first_contact_socket);
	}
	
	// Let other threads be created
	connect_hand.unlock();
	
	// Command loop
	server_comm.await_command_packet();
	cout << "liberating memory and ending thread for user: " + server_comm.get_username() << std::endl;
}


int main(int argc, char *argv[])
{
	// SERVER SOCKETS
	int server_socket;
	int first_contact_socket;
	struct sockaddr_in client_address;
	socklen_t client_len = sizeof(struct sockaddr_in);

	// Linked list to store client infos 
    ClientList client_device_list;
	ServerList server_list;

	if(argc == 1){
		cout << "Starting main server... \n";

		// BIND MAIN SOCKET
		serverStatus isBinded = bind_server_socket(&server_socket);
			if(isBinded != serverStatus::OK){
			cout << to_string(isBinded);
			return -1;
		}

		// LISTEN
		listen(server_socket, 6);
		cout << "================================\n SERVER LISTENING ON PORT 4000\n================================\n";

		// loop de criação de threads
		while(true){
			//pega o mutex para si, não deixando o while seguir aceitando outros clientes
			connect_hand.lock();

			//cria uma thread pra primeira conexão do cliente
			first_contact_socket = accept(server_socket,(struct sockaddr*)&client_address,&client_len);
			if(first_contact_socket >= 0){
				std::thread t(connection_handler, server_socket, first_contact_socket, &client_device_list, &server_list);
				t.detach();
			}
			
		}
	}else if(argc == 3){
		cout << "Starting backup server...\n";

		int  port;
    	struct hostent *server;
		bool elected = false;
    
		server = gethostbyname(argv[1]);
		port = atoi(argv[2]);
		setup_backup_server_socket(port, server, &server_socket);

		// Infinite loop awaiting syncronizations from main server
		while(!elected){
			serverComManager::await_sync(server_socket);
		}
	}


	return 0;
}

