#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "structures.h"

const int DATA_SEND_SIZE = 1024;
const int PACKET_DATA_MAX_SIZE = DATA_SEND_SIZE - sizeof(int) - sizeof(int); 
const int POLL_TIMEOUT = 500;

#endif