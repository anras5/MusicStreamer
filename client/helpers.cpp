#include "structures.h"
#include "constants.h"

#include <QTcpSocket>

void delete_packet(Packet &packet){
    if(packet.size > 0) {
        delete[] packet.data;
    }
}

bool send_packet_using_qt_socket(QTcpSocket &socket, Packet &packet) {

    char data[DATA_SEND_SIZE];
    //prepare data
    memcpy(data, &packet.type, sizeof(packet.size));
    memcpy(data + sizeof(packet.type), &packet.size, sizeof(packet.size));
    if(packet.size > 0)
    {
        memcpy(data + sizeof(packet.type) + sizeof(packet.size), packet.data, PACKET_DATA_MAX_SIZE);
    }

    // send it
    socket.write(data, DATA_SEND_SIZE);
    if(socket.waitForBytesWritten(300)) {
        return true;
    } else {
        return false;
    }
}

bool read_byte_array(const QByteArray &array, Packet &packet) {
    if(array.size()>=DATA_SEND_SIZE){
        packet.type = *((packetType*)(array.data()));
        packet.size = *((int*)(array.data()+sizeof(packetType)));
        packet.data = (char*)(array.data()+sizeof(packetType)+sizeof(int));
        return true;
    }
    return false;
}
