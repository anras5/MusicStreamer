#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <vector>
#include <poll.h>
#include <mutex>
#include <atomic>

enum packetType {
    P_AVAILABLE_SONGS_ASK,
    P_AVAILABLE_SONG,
    P_AVAILABLE_SONGS_END,
    P_UPLOAD_SONG_BEGIN,
    P_UPLOAD_SONG,
    P_UPLOAD_SONG_END,
    P_ADD_SONG_TO_QUEUE,
    P_SONG_IN_QUEUE,
    P_SONGS_IN_QUEUE_END,
    P_SONG_BEGIN,
    P_SONG,
    P_SONG_END,
    P_START_QUEUE,
    P_WANTS_TO_START,
    P_START,
    P_WANTS_TO_PAUSE,
    P_PAUSE,
    P_NEXT_SONG,
    P_UNLOCK_SQ_BTN,
    P_CLIENT_MODE,
    P_EMPTY_QUEUE,
    P_SKIP,
    P_SWAP
};

struct Packet {
    packetType type;
    int size;
    char* data;
};

struct Server {
    int sockfd;
    std::vector<pollfd> clients;
    std::vector<std::string> playQueue;

    bool hasQueueStarted;
    bool isPlaying;

    int howManyPlayersEnded = 0;
    int howManyPlayersPlay = 0;

};

#endif