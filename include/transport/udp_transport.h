#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

class UdpTransport : public QObject {
    Q_OBJECT
public:
    explicit UdpTransport(QObject* parent = nullptr);
    bool start(const QString& host, quint16 port);
    void stop();

signals:
    void dataReceived(const QByteArray& data);

private slots:
    void onReadyRead();

private:
    QUdpSocket* socket = nullptr;
};
