#include "model/telemetry_model.h"
#include <QDateTime>

TelemetryModel::TelemetryModel(QObject* parent) : QObject(parent) {}

void TelemetryModel::updateValue(uint32_t msgid, const std::string& param, double value) {
    auto ts = std::chrono::steady_clock::now();
    lastValues[{msgid, param}] = value;
    emit valueUpdated(msgid, param, value, ts);
}
