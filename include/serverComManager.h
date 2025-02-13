#ifndef SERVER_COM_MANAGER_H
#define SERVER_COM_MANAGER_H
#include <string>
#include <vector>
#include <tuple>
#include <sstream>
#include <mutex>
#include <thread>
#include <sys/stat.h>
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
#include "clientList.h"
#include "serverList.h"
#include "packet.h"
#include "serverStatus.h"
#include "serverFileManager.h"
#include "commandStatus.h"
#include "fileTransfer.h"

class serverComManager
{
    private: 
        ClientList* client_list;
        ServerList* server_list;
        serverFileManager file_manager;
        std::string username = "";
        std::string hostname = "";
        int client_cmd_socket = -1;
        int client_upload_socket= -1;
        int client_fetch_socket = -1;
        //sockets for the election ring
        int listening_socket = -1;
        int outgoing_election_socket = -1;
        int incoming_election_socket = -1;
        //bool that says if it is participating in the election or not
        bool participant = false;
        // int serving the id pupose in election
        int id = -1;

        // Communication Methods
        void start_communications();
        void end_communications(bool* exit);
        void get_sync_dir();
        void backup_sync_dir(int socket);
        void backup_server_list(int socket);
        void backup_client_list(int socket);
        void download(Packet command_packet);
        void list_server();
        void upload(Packet command_packet);
        void delete_server_file(Packet command_packet);
        static void election_timer(time_t* last_heartbeat, bool* should_start_election,int socket);

        /*functions used in backup_servers*/
        void connect_sockets(int port, hostent* client_address);
        void start_sockets();
        
    public:
        // Constructor Method
        serverComManager(ClientList* client_list, ServerList* server_list);
        void await_command_packet();
        void await_sync(int* socket, bool* elected);
        static void heartbeat_protocol(ServerList* server_list);
        serverStatus bind_client_sockets(int server_socket, int first_comm_socket);
        void add_backup_server(int backup_server_socket, string hostname);
        void receive_server_list(int socket);
        void receive_client_list(int socket);
        std::string get_username();

        /*functions used in backup_servers*/
        void connect_to_client_hostname(const char *hostname, string username);
        void connect_to_server_hostname(const char *hostname);
        //functions used for the election in the backup_servers
        
        void start_election_sockets();
        void bind_incoming_election_socket();
        void accept_election_connection();
        void connect_election_sockets(hostent* backup_server);
        void start_ring_election(bool* wait_election);
        void handle_election(bool* wait_election, bool* elected, bool* reconnected, int* socket);
        void build_ring();

        //functions used for evolvin backup server
        void evolve_into_main();
        void close_old_connections();
        void reconnect_to_clients();
        void reconnect_to_servers();
        void await_main_server_connection(int* socket);
        
};
#endif