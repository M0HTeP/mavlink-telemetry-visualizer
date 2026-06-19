#include "history/history_manager.h"
#include <algorithm>

HistoryManager::HistoryManager(QObject* parent) : QObject(parent) {
    const auto& cfg = getAppConfig();
    maxRecords = cfg.history.max_records_per_message;
    maxAgeSeconds = cfg.history.max_age_seconds;
    stopWorker = false;
    workerThread = std::thread(&HistoryManager::workerLoop, this);
}

HistoryManager::~HistoryManager() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopWorker = true;
    }
    queueCondition.notify_one();
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void HistoryManager::addData(uint32_t msgid, const std::string& param, double value, TimePoint ts) {
    SeriesKey key{msgid, param};
    HistoryRecord record{key, TelemetryPoint{ts, value}};
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push_back(std::move(record));
        if (queue.size() > maxRecords * 4) {
            queue.pop_front();
        }
    }
    queueCondition.notify_one();
}

void HistoryManager::onValueUpdated(uint32_t msgid, const std::string& param, double value, TimePoint ts) {
    addData(msgid, param, value, ts);
}

void HistoryManager::workerLoop() {
    while (true) {
        HistoryRecord record;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this]() {
                return stopWorker || !queue.empty();
            });
            if (stopWorker && queue.empty()) {
                return;
            }
            record = std::move(queue.front());
            queue.pop_front();
        }
        processRecord(record.key, record.point);
    }
}

void HistoryManager::processRecord(const SeriesKey& key, const TelemetryPoint& point) {
    std::lock_guard<std::mutex> lock(mutex);
    auto& deque = history[key];
    deque.push_back(point);
    if (deque.size() > maxRecords) {
        deque.pop_front();
    }
    if (maxAgeSeconds > 0) {
        const auto cutoff = point.timestamp - std::chrono::seconds(maxAgeSeconds);
        while (!deque.empty() && deque.front().timestamp < cutoff) {
            deque.pop_front();
        }
    }
}

std::vector<SeriesKey> HistoryManager::getAvailableSeries() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<SeriesKey> keys;
    keys.reserve(history.size());
    for (const auto& p : history) {
        keys.push_back(p.first);
    }
    return keys;
}

std::vector<TelemetryPoint> HistoryManager::getData(const SeriesKey& key, TimePoint start, TimePoint end) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = history.find(key);
    if (it == history.end()) return {};
    std::vector<TelemetryPoint> result;
    result.reserve(it->second.size());
    for (const auto& p : it->second) {
        if (p.timestamp >= start && p.timestamp <= end) {
            result.push_back(p);
        }
    }
    return result;
}
