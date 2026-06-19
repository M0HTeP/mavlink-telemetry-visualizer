#pragma once
#include <QObject>
#include "common/telemetry_types.h"
#include "common/config.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

class HistoryManager : public QObject {
    Q_OBJECT
public:
    explicit HistoryManager(QObject* parent = nullptr);
    ~HistoryManager() override;

    void addData(uint32_t msgid, const std::string& param, double value, TimePoint ts);
    std::vector<SeriesKey> getAvailableSeries() const;
    std::vector<TelemetryPoint> getData(const SeriesKey& key, TimePoint start, TimePoint end) const;

public slots:
    void onValueUpdated(uint32_t msgid, const std::string& param, double value, TimePoint ts);

private:
    void workerLoop();
    void processRecord(const SeriesKey& key, const TelemetryPoint& point);

    struct HistoryRecord {
        SeriesKey key;
        TelemetryPoint point;
    };

    mutable std::mutex mutex;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::deque<HistoryRecord> queue;
    std::thread workerThread;
    bool stopWorker = false;

    std::unordered_map<SeriesKey, std::deque<TelemetryPoint>> history;
    size_t maxRecords = 10000;
    int maxAgeSeconds = 600;
};
