#pragma once
#include <QObject>
#include "common/telemetry_types.h"

class TelemetryModel : public QObject {
    Q_OBJECT
public:
    explicit TelemetryModel(QObject* parent = nullptr);

    void updateValue(uint32_t msgid, const std::string& param, double value);

signals:
    void valueUpdated(uint32_t msgid, const std::string& param, double value, TimePoint ts);

private:
    std::unordered_map<SeriesKey, double> lastValues;
};
