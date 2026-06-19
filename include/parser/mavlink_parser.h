#pragma once
#include <QObject>
#include "common/telemetry_types.h"
#include "mavlink_c_library_v2/common/mavlink.h"

class MavlinkParser : public QObject {
    Q_OBJECT
public:
    explicit MavlinkParser(QObject* parent = nullptr);

signals:
    void messageParsed(uint32_t msgid, const void* payload);

public slots:
    void processData(const QByteArray& data);

private:
    mavlink_message_t msg;
    mavlink_status_t status;
};
