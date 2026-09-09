#include "DatabaseConfig.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <spdlog/spdlog.h>

namespace sms::config {

namespace {

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool parseAndApplyDotEnv(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trim(line.substr(0, eqPos));
        std::string val = trim(line.substr(eqPos + 1));

        if (key.empty()) continue;

        // Strip quotes if present
        if (val.size() >= 2 && 
            ((val.front() == '"' && val.back() == '"') || 
             (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.size() - 2);
        }

        // Only set if not already set in environment
        if (!std::getenv(key.c_str())) {
#if defined(_WIN32)
            _putenv_s(key.c_str(), val.c_str());
#else
            setenv(key.c_str(), val.c_str(), 0);
#endif
        }
    }
    return true;
}

} // anonymous namespace

bool DatabaseConfig::loadDotEnv(const std::string& explicitPath) {
    if (!explicitPath.empty()) {
        std::filesystem::path path(explicitPath);
        if (std::filesystem::exists(path) && parseAndApplyDotEnv(path)) {
            spdlog::info("Database config loaded environment variables from: {}", path.string());
            return true;
        }
        return false;
    }

    std::vector<std::filesystem::path> candidates = {
        ".env",
        "../.env",
        "../../.env"
    };

    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            if (parseAndApplyDotEnv(path)) {
                spdlog::info("Database config loaded environment variables from: {}", path.string());
                return true;
            }
        }
    }
    return false;
}

std::string DatabaseConfig::getEnv(const std::string& key, const std::string& defaultValue) {
    const char* value = std::getenv(key.c_str());
    return value ? std::string(value) : defaultValue;
}

std::string DatabaseConfig::getConnectionString() const {
    if (!m_connectionString.empty()) {
        return m_connectionString;
    }
    std::string connStr = "postgresql://" + m_username;
    if (!m_password.empty()) {
        connStr += ":" + m_password;
    }
    connStr += "@" + m_host + ":" + std::to_string(m_port) + "/" + m_database;
    if (!m_sslMode.empty()) {
        connStr += "?sslmode=" + m_sslMode;
    }
    return connStr;
}

std::unique_ptr<DatabaseConfig> DatabaseConfig::loadFromEnvironment() {
    loadDotEnv();

    auto config = std::unique_ptr<DatabaseConfig>(new DatabaseConfig());
    
    std::string dbUrl = getEnv("DATABASE_URL", getEnv("DB_URL", ""));
    if (!dbUrl.empty()) {
        config->m_connectionString = dbUrl;
        config->m_poolSize = std::stoi(getEnv("DB_POOL_SIZE", "10"));
        spdlog::info("Database config loaded from DATABASE_URL / DB_URL environment variable");
        return config;
    }

    config->m_host = getEnv("DB_HOST", "localhost");
    config->m_port = std::stoi(getEnv("DB_PORT", "5432"));
    config->m_database = getEnv("DB_NAME", "student_db");
    config->m_username = getEnv("DB_USER", "postgres");
    config->m_password = getEnv("DB_PASSWORD", "");
    config->m_sslMode = getEnv("DB_SSLMODE", "");
    config->m_poolSize = std::stoi(getEnv("DB_POOL_SIZE", "10"));

    spdlog::info("Database config loaded: {}:{}/{}", 
        config->m_host, config->m_port, config->m_database);

    return config;
}

} // namespace sms::config
