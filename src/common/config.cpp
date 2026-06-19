#include "common/config.h"
#include <algorithm>
#include <fstream>
#include <iostream>

static std::optional<fs::path> findConfigFile() {
    std::vector<fs::path> candidates;
    const fs::path cwd = fs::current_path();

    for (fs::path path = cwd; !path.empty(); path = path.parent_path()) {
        candidates.emplace_back(path / "config.json");
        if (path == path.parent_path()) break;
    }

    const fs::path procExe = "/proc/self/exe";
    if (fs::exists(procExe)) {
        try {
            const fs::path exePath = fs::read_symlink(procExe);
            fs::path exeDir = exePath.parent_path();
            for (fs::path path = exeDir; !path.empty(); path = path.parent_path()) {
                candidates.emplace_back(path / "config.json");
                if (path == path.parent_path()) break;
            }
        } catch (const std::filesystem::filesystem_error&) {
            // ignore if /proc/self/exe cannot be read
        }
    }

    std::vector<fs::path> validPaths;
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && fs::exists(candidate) && fs::is_regular_file(candidate)) {
            try {
                validPaths.emplace_back(fs::weakly_canonical(candidate));
            } catch (const std::filesystem::filesystem_error&) {
                continue;
            }
        }
    }

    std::sort(validPaths.begin(), validPaths.end(), [](const fs::path& a, const fs::path& b) {
        return std::distance(a.begin(), a.end()) < std::distance(b.begin(), b.end());
    });
    validPaths.erase(std::unique(validPaths.begin(), validPaths.end()), validPaths.end());

    if (!validPaths.empty()) {
        return validPaths.front();
    }
    return std::nullopt;
}

void from_json(const nlohmann::json& j, AppConfig& c) {
    if (j.contains("udp")) {
        auto u = j["udp"];
        if (u.contains("host")) c.udp.host = u["host"].get<std::string>();
        if (u.contains("port")) c.udp.port = u["port"].get<int>();
    }
    if (j.contains("history")) {
        auto h = j["history"];
        if (h.contains("max_records_per_message")) c.history.max_records_per_message = h["max_records_per_message"].get<size_t>();
        if (h.contains("max_age_seconds")) c.history.max_age_seconds = h["max_age_seconds"].get<int>();
    }
    if (j.contains("chart")) {
        auto ch = j["chart"];
        if (ch.contains("window_seconds")) c.chart.window_seconds = ch["window_seconds"].get<int>();
        if (ch.contains("update_ms")) c.chart.update_ms = ch["update_ms"].get<int>();
            if (ch.contains("live_restore_ms")) c.chart.live_restore_ms = ch["live_restore_ms"].get<int>();
    }
}

void to_json(nlohmann::json& j, const AppConfig& c) {
    j = nlohmann::json{
        {"udp", { {"host", c.udp.host}, {"port", c.udp.port} }},
        {"history", { {"max_records_per_message", c.history.max_records_per_message}, {"max_age_seconds", c.history.max_age_seconds} }},
        {"chart", { {"window_seconds", c.chart.window_seconds}, {"update_ms", c.chart.update_ms}, {"live_restore_ms", c.chart.live_restore_ms} }}
    };
}

AppConfig loadConfig() {
    AppConfig cfg;
    const auto configPath = findConfigFile();
    if (!configPath) {
        std::cerr << "Warning: config.json not found, using defaults\n";
        return cfg;
    }

    std::cerr << "Loading config from " << configPath->string() << "\n";
    std::ifstream config_file(configPath->string());
    if (!config_file.is_open()) {
        std::cerr << "Warning: failed to open " << configPath->string() << ", using defaults\n";
        return cfg;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(config_file);
        cfg = j.get<AppConfig>();
    } catch (const std::exception& e) {
        std::cerr << "Warning: failed to parse config.json: " << e.what() << "\n";
    }

    return cfg;
}

// Define a global config instance. It is loaded explicitly at startup from main().
AppConfig gAppConfig;

const AppConfig& getAppConfig() {
    return gAppConfig;
}
