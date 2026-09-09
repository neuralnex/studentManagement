#pragma once

#include <string>
#include <optional>
#include <memory>

namespace sms::config {

/**
 * @brief Database configuration holder
 * 
 * Loads database connection parameters from environment variables
 * following the 12-factor app methodology.
 */
class DatabaseConfig {
public:
    /**
     * @brief Get host address
     * @return Database host
     */
    [[nodiscard]] const std::string& getHost() const { return m_host; }

    /**
     * @brief Get port number
     * @return Database port
     */
    [[nodiscard]] int getPort() const { return m_port; }

    /**
     * @brief Get database name
     * @return Database name
     */
    [[nodiscard]] const std::string& getDatabase() const { return m_database; }

    /**
     * @brief Get username
     * @return Database user
     */
    [[nodiscard]] const std::string& getUsername() const { return m_username; }

    /**
     * @brief Get password
     * @return Database password
     */
    [[nodiscard]] const std::string& getPassword() const { return m_password; }

    /**
     * @brief Get connection pool size
     * @return Number of connections to maintain
     */
    [[nodiscard]] int getPoolSize() const { return m_poolSize; }

    /**
     * @brief Build connection string
     * @return PostgreSQL connection string
     */
    [[nodiscard]] std::string getConnectionString() const;

    /**
     * @brief Load environment variables from a .env file
     * @param path Optional explicit path to .env file
     * @return True if file was found and loaded
     */
    static bool loadDotEnv(const std::string& path = "");

    /**
     * @brief Load configuration from environment
     * @return True if configuration loaded successfully
     */
    static std::unique_ptr<DatabaseConfig> loadFromEnvironment();

private:
    DatabaseConfig() = default;

    std::string m_host{"localhost"};
    int m_port{5432};
    std::string m_database{"student_db"};
    std::string m_username{"postgres"};
    std::string m_password{};
    std::string m_sslMode{};
    std::string m_connectionString{};
    int m_poolSize{10};

    /**
     * @brief Get environment variable with fallback
     * @param key Variable name
     * @param defaultValue Fallback value
     * @return Environment variable value or default
     */
    static std::string getEnv(const std::string& key, const std::string& defaultValue = "");
};

} // namespace sms::config
