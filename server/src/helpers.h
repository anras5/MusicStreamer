#ifndef HELPERS_H
#define HELPERS_H

#include "structures.h"
#include "constants.h"

void delete_packet(Packet &packet);

bool ends_with(std::string const &fullString, std::string const &ending);

bool send_packet_using_socket(int socket, Packet &packet);

bool receive_packet_using_socket(int socket, Packet &packet);

#endif