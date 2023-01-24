#include <iostream>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "structures.h"
#include "constants.h"

void delete_packet(Packet &packet){
    if(packet.size > 0) {
        delete[] packet.data;
    }
}


bool send_packet_using_socket(int socket, Packet &packet) {

    char data[DATA_SEND_SIZE];

    pollfd desc[1];

    desc[0].fd = socket;
    desc[0].events = POLLOUT;

    int ready = poll(desc, 1, POLL_TIMEOUT);
    if (ready == -1) {
        return false;
    }
    if (desc[0].revents & POLLOUT)
    {
        // prepare data
        memcpy(data, &packet.type, sizeof(packet.size));
        memcpy(data + sizeof(packet.type), &packet.size, sizeof(packet.size));
        if(packet.size > 0)
        {
            memcpy(data + sizeof(packet.type) + sizeof(packet.size), packet.data, PACKET_DATA_MAX_SIZE);
        }

        // send it
        int bytesSent = write(socket, data, DATA_SEND_SIZE);
        if(bytesSent == -1){
            return false;
        }
        return true;
    }
    return false;
}

bool receive_packet_using_socket(int socket, Packet &packet) {

    char data[DATA_SEND_SIZE];

    pollfd desc[1];
    desc[0].fd = socket;
    desc[0].events = POLLIN;

    int ready = poll(desc, 1, POLL_TIMEOUT);
    if(ready == -1) {
        return false;
    }
    if(desc[0].revents & POLLIN) {

        //read the data
        int readBytes = read(socket, data, DATA_SEND_SIZE);
        if(readBytes > 0) {

            //read whole data
            int bytesPointer = readBytes;
            while(readBytes < DATA_SEND_SIZE && readBytes > 0) {
                readBytes += read(socket, data + bytesPointer, DATA_SEND_SIZE - bytesPointer);
                bytesPointer = readBytes;
            }

            if(readBytes == DATA_SEND_SIZE) {
                memcpy(&packet.type, data, sizeof(packet.type));
                memcpy(&packet.size, data + sizeof(packet.type), sizeof(packet.size));
                if(packet.size > 0) {
                    packet.data = new char[PACKET_DATA_MAX_SIZE];
                    memcpy(packet.data, data + sizeof(packet.type) + sizeof(packet.size), packet.size);
                }
                return true;
            }
        }
    }
    return false;
}