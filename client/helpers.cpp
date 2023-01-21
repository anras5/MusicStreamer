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
        memcpy(data + sizeof(packet.type) + sizeof(packet.size), packet.data, DATA_SEND_SIZE);
    }

    // send it
    socket.write(data, DATA_SEND_SIZE);
    if(socket.waitForBytesWritten(300)) {
        return true;
    } else {
        return false;
    }
}

//bool receive_packet_using_qt_socket(QTcpSocket &socket, Packet &packet) {

//    char data[256];

//    if(socket.waitForReadyRead(1) || socket.bytesAvailable()) {
//        int bytes = socket.read(data, 256);
//        if(bytes > 0) {
//            int bytesPointer = bytes;
//            while(bytes < 256 && bytes > 0) {
//                bytes += socket.read(data + bytesPointer, 256 - bytesPointer);
//                bytesPointer = bytes;
//            }

//            if(bytes == 256) {
//                memcpy(&packet.type, data, sizeof(packet.type));
//                memcpy(&packet.size, data + sizeof(packet.type), sizeof(packet.size));
//                if(packet.size > 0) {
//                    packet.data = new char[256];
//                    memcpy(packet.data, data + sizeof(packet.type) + sizeof(packet.size), packet.size);

//                }
//                return true;
//            }
//        }
//    }
//    return false;
//}

bool read_byte_array(const QByteArray &array, Packet &packet) {
    if(array.size()>=DATA_SEND_SIZE){
        packet.type = *((packetType*)(array.data()));
        packet.size = *((int*)(array.data()+sizeof(packetType)));
        packet.data = (char*)(array.data()+sizeof(packetType)+sizeof(int));
        return true;
    }
    return false;
}
