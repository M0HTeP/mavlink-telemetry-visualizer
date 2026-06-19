#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

struct AppConfig {
    struct Udp {
        std::string host = "127.0.0.1";
        int port = 14550;
    } udp;

    struct History {
        size_t max_records_per_message = 10000;
        int max_age_seconds = 600;
    } history;

    struct Chart {
        int window_seconds = 300;
        int update_ms = 200;
        int live_restore_ms = 5000;
    } chart;
};

void from_json(const nlohmann::json& j, AppConfig& c);
void to_json(nlohmann::json& j, const AppConfig& c);
AppConfig loadConfig();

// Global configuration instance populated by `loadConfig()` at program start.
// Use `getAppConfig()` to access parsed config anywhere in the codebase.
extern AppConfig gAppConfig;
const AppConfig& getAppConfig();
