#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <poll.h>
#include <thread>
#include <regex>
#include "structures.h"
#include "helpers.h"


const int noClients = 5;

// create_socket creates socket, binds it, makes it listen and returns the socket's fd
int create_socket(int argc, char* argv[]) {

    // Read the port from argv
    if (argc < 2) {
        std::cerr << "Error: no port number provided" << std::endl;
        return -1;
    } 
    int port = std::stoi(argv[1]);

    // Create the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }


    // Bind the socket to an IP and port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    // Listen for incoming connections
    listen(sockfd, noClients);

    return sockfd;
}

void send_available_songs(int socket) {
    // Open the directory
    DIR* dir = opendir("musicServer/");
    dirent* entry;

    // Iterate over the entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        Packet packet;
        packet.type = P_AVAILABLE_SONG;
        packet.size = sizeof(entry->d_name);
        packet.data = entry->d_name;

        bool isPacketSent = false;
		while(!isPacketSent)
		{
            std::cout << entry->d_name << std::endl;
            isPacketSent = send_packet_using_socket(socket, packet);
		}

    }
    // send end message
    Packet packet;
    packet.type = P_AVAILABLE_SONGS_END;
    packet.size = 0;
    send_packet_using_socket(socket, packet);
}

void save_new_song(int socket, Packet &packetWithName) {

    std::string fileName = packetWithName.data;
    std::string allowedDirectory = "musicServer/"; 
    std::string filePath = allowedDirectory + fileName;

    // Check for invalid characters in fileName
    if (!std::regex_match(fileName, std::regex("^[A-Za-z0-9_.]+$"))) {
        std::cerr << "Error: Invalid characters in file name" << std::endl;
        return;
    }

    bool isFileOpen = true;

    // Open the .wav file
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        isFileOpen = false;
    }   


    Packet packet;
    while(receive_packet_using_socket(socket, packet)) {
        if(packet.type == P_UPLOAD_SONG_END){
            std::cout << "ending saving new song" << std::endl;
            break;
        }
        if(isFileOpen)
            file.write(packet.data, packet.size);     
    }
    delete_packet(packet);

    file.close();
}

void send_song_in_queue_name(int socket, std::string songName) {
    Packet packet;
    packet.type = P_SONG_IN_QUEUE;
    packet.size = songName.length();
    packet.data = const_cast<char*>(songName.c_str());
    send_packet_using_socket(socket, packet);
}

void send_songs_in_queue_end(int socket) {
    Packet packet;
    packet.type = P_SONGS_IN_QUEUE_END;
    packet.size = 0;
    send_packet_using_socket(socket, packet);
}

void send_queue_to_client(int socket, Server &server) {
    if(!server.playQueue.empty()){
        for(long unsigned int x=0; x < server.playQueue.size(); x++) {
            send_song_in_queue_name(socket, server.playQueue[x]);
        }
        send_songs_in_queue_end(socket);
    }
}

void send_empty_queue_to_client(int socket) {
    Packet packet;
    packet.type = P_EMPTY_QUEUE;
    packet.size = 0;
    send_packet_using_socket(socket, packet);
}

void send_mode_to_client(int socket, Server &server) {
    std::string w = "WAIT";
    std::string a = "ACTIVE";
    Packet packet;
    packet.type = P_CLIENT_MODE;
    if(server.hasQueueStarted) {
        packet.data = const_cast<char*>(w.c_str());
        packet.size = sizeof(w.length());
    } else {
        packet.data = const_cast<char*>(a.c_str());
        packet.size = sizeof(a.length());
    }
    send_packet_using_socket(socket, packet);
}

void send_queue_to_all_clients(Server &server) {
    std::cout << "Sending queue to all clients" << std::endl;
    if(!server.playQueue.empty() && !server.clients.empty()) {
        for(long unsigned int i=1; i < server.clients.size(); i++) {
            send_queue_to_client(server.clients[i].fd, server);
        }
    } else if(!server.clients.empty()) {
        for(long unsigned int i=1; i < server.clients.size(); i++) {
            send_empty_queue_to_client(server.clients[i].fd);
        }
    }
}

void add_new_song_to_queue(Packet &packetWithName, Server &server) {
    
    std::string fileName = packetWithName.data;
    std::cout << "Adding " << fileName << " to the end of the queue" << std::endl;
    if(ends_with(fileName, ".wav")){
        server.playQueue.push_back(fileName);
        send_queue_to_all_clients(server);
    }

} 

void send_song_to_client(int socket, std::string fileName, int startTime) {
    std::ifstream file("musicServer/" + fileName, std::ios::binary);    
    if(!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    // send P_SONG_BEGIN
    Packet packet;
    packet.type = P_SONG_BEGIN;
    packet.size = 0;
    send_packet_using_socket(socket, packet);

    // send P_SONG
    int qwe = 0 ;
    while(true) {
        packet.type = P_SONG;
        packet.size = PACKET_DATA_MAX_SIZE;
        packet.data = new char[PACKET_DATA_MAX_SIZE];
        file.read(packet.data, PACKET_DATA_MAX_SIZE);
        int bytesRead = file.gcount();
        if(bytesRead == 0) {
            std::cout << "Whole file read" << std::endl;
            delete_packet(packet);
            break;
        }
        if(!send_packet_using_socket(socket, packet)) {
            std::cout << "File could not be sent" << std::endl;
            delete_packet(packet);
            break;
        }
        delete_packet(packet);
        qwe++;
    }

    file.close();

    std::cout << "File sent in " << qwe << " packets" << std::endl;

    packet.type = P_SONG_END;
    std::string numString;
    std::stringstream ss;
    ss << startTime;
    numString = ss.str();
    packet.size = numString.length();
    packet.data = const_cast<char*>(numString.c_str());
    send_packet_using_socket(socket, packet);
}

void send_first_song_to_all_clients(Server &server) {

    server.howManyPlayersPlay = 0;
    server.howManyPlayersEnded = 0;
    if(server.playQueue.size() > 0) {
        for(long unsigned int i=1;i < server.clients.size();i++) {
            send_song_to_client(server.clients[i].fd, server.playQueue[0], 0);
            server.howManyPlayersPlay++;
        }
        server.hasQueueStarted = true;
        server.isPlaying = true;
    }

}

void send_unlock_start_queue_button(int clientSocket) {
    Packet packet;
    packet.type = P_UNLOCK_SQ_BTN;
    packet.size = 0;
    send_packet_using_socket(clientSocket, packet);
}

void send_unlock_start_queue_button_to_all_clients(Server &server) {

    for(long unsigned int i=1;i < server.clients.size();i++) {
        send_unlock_start_queue_button(server.clients[i].fd);
    }

}

void clear_songs_queue(Server &server) {
    std::cout << "clients now: " << server.clients.size() << std::endl;
    if(server.clients.size() == 1) { // only server socket
        server.playQueue.clear();
        server.isPlaying = false;
        server.hasQueueStarted = false;
    }
}

void delete_client(Server &server, int numberInClients) {
    server.clients.erase(server.clients.begin() + numberInClients);
    clear_songs_queue(server);
}

void handle_next_song_request(Server &server) {
    
    server.howManyPlayersEnded++;
    if(server.howManyPlayersEnded >= server.howManyPlayersPlay) {        // if everyone sent that wants next song
        server.playQueue.erase(server.playQueue.begin());                // delete the first song from queue
        if(server.playQueue.size() > 0) {                                // if there is next song to play                      
            send_queue_to_all_clients(server);                           // send new queue to all clients
            send_first_song_to_all_clients(server);                      // send first song to all clients
        } else { 
            send_queue_to_all_clients(server);                           // send empty queue to all clients
            send_unlock_start_queue_button_to_all_clients(server);       // send info to unlock the start queue button
            server.isPlaying = false;
            server.hasQueueStarted = false;
        }
    }
    server.howManyPlayersEnded = 0;

}

void skip_current(Server &server) {

    if(server.playQueue.size() > 0) {
        server.playQueue.erase(server.playQueue.begin());
        if(server.playQueue.size() > 0) {                                // if there is next song to play                      
            send_queue_to_all_clients(server);                           // send new queue to all clients
            send_first_song_to_all_clients(server);                      // send first song to all clients
        } else { 
            send_queue_to_all_clients(server);                           // send empty queue to all clients
            send_unlock_start_queue_button_to_all_clients(server);       // send info to unlock the start queue button
            server.isPlaying = false;
            server.hasQueueStarted = false;
        }
    }
    server.howManyPlayersEnded = 0;

}

void send_pause_client(int socket) {
    Packet packet;
    packet.type = P_PAUSE;
    packet.size = 0;
    send_packet_using_socket(socket, packet);
}

void send_pause_to_all_clients(Server &server) {
    if(server.isPlaying == true) {
        for(long unsigned int i=1;i<server.clients.size();i++){
            send_pause_client(server.clients[i].fd);
        }
    }
    server.isPlaying = false;
}

void send_play_client(int socket) {
    Packet packet;
    packet.type = P_START;
    packet.size = 0;
    send_packet_using_socket(socket, packet);
}

void send_play_to_all_clients(Server &server) {
    if(server.isPlaying == false) {
        for(long unsigned int i=1;i<server.clients.size();i++){
            send_play_client(server.clients[i].fd);
        }
    }
    server.isPlaying = true;
}

void swap_songs(Server &server, Packet &packet) {
    // get values from packet
    int a, b;
    char* token = strtok(packet.data, "#");
    a = atoi(token);
    token = strtok(NULL, "#");
    b = atoi(token);


    if(a == 1) {
        std::iter_swap(server.playQueue.begin(), server.playQueue.begin() + b - 1);
        send_queue_to_all_clients(server);
        send_first_song_to_all_clients(server);
    } else {
        std::iter_swap(server.playQueue.begin() + a - 1, server.playQueue.begin() + b - 1);
        send_queue_to_all_clients(server);  
    }
}

bool event_on_client(int numberInClients, Server &server) {

    int clientSocket = server.clients[numberInClients].fd;
    auto revents = server.clients[numberInClients].revents;

    if (revents & POLLIN) {

        Packet packet;
        bool isReceived = receive_packet_using_socket(clientSocket, packet);
        if(!isReceived) {
            std::cout << "Client with socket " << clientSocket << " erased from clients" << std::endl;
            delete_client(server, numberInClients);
            return false;
        }

        // client asks for a list of songs
        if(packet.type == P_AVAILABLE_SONGS_ASK){
            std::cout << "Client wants to see available songs" << std::endl;
            send_available_songs(clientSocket);
            delete_packet(packet);
            return true;
        }

        // client sends new song
        if(packet.type == P_UPLOAD_SONG_BEGIN) {
            std::cout << "Client wants to see new song" << std::endl;
            save_new_song(clientSocket, packet);
            delete_packet(packet);
            return true;
        }

        // client adds new song to queue
        if(packet.type == P_ADD_SONG_TO_QUEUE) {
            std::cout << "Client wants to add new song to queue" << std::endl;
            add_new_song_to_queue(packet, server);
            delete_packet(packet);
            return true;
        }

        // client wants to start the queue
        if(packet.type == P_START_QUEUE) {
            std::cout << "Client wants to start the queue" << std::endl;
            send_first_song_to_all_clients(server);
            return true;
        }

        if(packet.type == P_NEXT_SONG) {
            std::cout << "Client wants to start next song" << std::endl;
            handle_next_song_request(server);
            return true;
        }

        if(packet.type == P_WANTS_TO_PAUSE) {
            std::cout << "Client wants to pause playing" << std::endl;
            send_pause_to_all_clients(server);
            return true;
        }

        if(packet.type == P_WANTS_TO_START) {
            std::cout << "Client wants to start playing" << std::endl;
            send_play_to_all_clients(server);
            return true;
        }

        if(packet.type == P_SKIP) {
            std::cout << "Client wants to skip" << std::endl;
            skip_current(server);
            return true;
        }

        if(packet.type == P_SWAP) {
            std::cout << "Client wants to swap" << std::endl;
            swap_songs(server, packet);
            delete_packet(packet);
            return true;
        }

        // std::cout << "Some other type of request, no. " << packet.type << std::endl;
    }
    return true;
}

int event_on_server(int revents, Server &server) {
    // new client
    if(revents & POLLIN) {
        // Accept incoming connections
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int clientSockfd = accept(server.sockfd , (struct sockaddr *) &client_address, &client_len);
        if (clientSockfd < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            return -1;
        }
        // add client to server's clients
        pollfd temp_poll_str;
        temp_poll_str.fd = clientSockfd;
        temp_poll_str.events = POLLIN;

        server.clients.push_back(temp_poll_str);
        std::cout << "New client " << clientSockfd << std::endl;

        // send song queue
        send_queue_to_client(clientSockfd, server);

        // send mode
        send_mode_to_client(clientSockfd, server);

        return clientSockfd;
    }
    return -1;
}

int main(int argc, char* argv[]) {
    // Create the server structure
    Server server;
    server.sockfd = create_socket(argc, argv);
    if(server.sockfd == -1) {
        return 1;
    }
    pollfd temp;
    temp.fd = server.sockfd;
    temp.events = POLLIN;
    server.clients.push_back(temp);

    while (true) {
        int ready = poll(&server.clients[0], server.clients.size(), -1);
        if (ready == -1) {
            std::cerr << "Error: poll not ready in main function" << std::endl;
        }

        for(long unsigned int i=0; i < server.clients.size() && ready > 0 ; i++) {
            if(server.clients[i].revents) {
                if(server.clients[i].fd == server.sockfd){
                    std::cout << "event on server" << std::endl;
                    event_on_server(server.clients[i].revents, server);
                }
                else{
                    event_on_client(i, server);
                }
                ready--;
            }
        }
    }

    // Close the server socket
    close(server.sockfd);

    return 0;
}
