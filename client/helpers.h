#ifndef HELPERS_H
#define HELPERS_H
#include <QTcpSocket>
#include <QByteArray>
#include "structures.h"


void delete_packet(Packet &packet);

bool send_packet_using_qt_socket(QTcpSocket &socket, Packet &packet);

//bool receive_packet_using_qt_socket(QTcpSocket &socket, Packet &packet);

bool read_byte_array(const QByteArray &array, Packet &packet);

#endif // HELPERS_H
