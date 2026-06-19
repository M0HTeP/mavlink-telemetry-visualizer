#pragma once
#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <deque>
#include <mutex>

using TimePoint = std::chrono::steady_clock::time_point;

struct TelemetryPoint {
    TimePoint timestamp;
    double value;
};

struct SeriesKey {
    uint32_t message_id;
    std::string param_name;

    bool operator==(const SeriesKey& other) const {
        return message_id == other.message_id && param_name == other.param_name;
    }
};

namespace std {
    template<> struct hash<SeriesKey> {
        size_t operator()(const SeriesKey& k) const {
            return std::hash<uint32_t>()(k.message_id) ^ std::hash<std::string>()(k.param_name);
        }
    };
}
