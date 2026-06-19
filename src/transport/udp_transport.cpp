#include "transport/udp_transport.h"

UdpTransport::UdpTransport(QObject* parent) : QObject(parent) {}

bool UdpTransport::start(const QString& host, quint16 port) {
    socket = new QUdpSocket(this);
    connect(socket, &QUdpSocket::readyRead, this, &UdpTransport::onReadyRead);
    return socket->bind(QHostAddress::Any, port);  // Listen on any for SITL
}

void UdpTransport::stop() {
    if (socket) socket->close();
}

void UdpTransport::onReadyRead() {
    while (socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(socket->pendingDatagramSize()));
        socket->readDatagram(datagram.data(), datagram.size());
        emit dataReceived(datagram);
    }
}
