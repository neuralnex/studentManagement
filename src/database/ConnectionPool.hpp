#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <pqxx/pqxx>
#include "../config/DatabaseConfig.hpp"

namespace sms::database {

/**
 * @brief Thread-safe connection pool for PostgreSQL
 * 
 * Manages a pool of database connections to minimize overhead
 * of creating/destroying connections. Implements RAII semantics
 * for automatic connection release.
 */
class ConnectionPool {
public:
    /**
     * @brief RAII connection wrapper
     * 
     * Automatically returns connection to pool when destroyed.
     */
    class Connection {
    public:
        Connection(ConnectionPool& pool, std::unique_ptr<pqxx::connection> conn)
            : m_pool(pool), m_connection(std::move(conn)) {}

        ~Connection() {
            m_pool.releaseConnection(std::move(m_connection));
        }

        /**
         * @brief Get underlying connection
         * @return Pointer to pqxx::connection
         */
        pqxx::connection* operator->() { return m_connection.get(); }
        pqxx::connection& operator*() { return *m_connection; }

        /**
         * @brief Check if connection is valid
         * @return True if connection is open
         */
        bool isOpen() const { return m_connection && m_connection->is_open(); }

    private:
        ConnectionPool& m_pool;
        std::unique_ptr<pqxx::connection> m_connection;
    };

    /**
     * @brief Initialize connection pool
     * @param config Database configuration
     * @throws std::runtime_error if pool initialization fails
     */
    explicit ConnectionPool(const std::shared_ptr<config::DatabaseConfig>& config);

    /**
     * @brief Destructor - closes all connections
     */
    ~ConnectionPool();

    /**
     * @brief Get a connection from the pool
     * @return RAII-wrapped connection
     * @throws std::runtime_error if no connections available
     */
    Connection acquireConnection();

    /**
     * @brief Get current pool size
     * @return Number of active connections
     */
    [[nodiscard]] size_t getCurrentSize() const;

    /**
     * @brief Get maximum pool size
     * @return Maximum connections allowed
     */
    [[nodiscard]] size_t getMaxSize() const { return m_maxPoolSize; }

    /**
     * @brief Check if pool is healthy
     * @return True if pool is operational
     */
    [[nodiscard]] bool isHealthy() const;

    /**
     * @brief Reset pool (close all connections)
     */
    void reset();

    // Prevent copying
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    // Allow moving
    ConnectionPool(ConnectionPool&&) = default;
    ConnectionPool& operator=(ConnectionPool&&) = default;

private:
    /**
     * @brief Create a new connection
     * @return New database connection
     */
    std::unique_ptr<pqxx::connection> createConnection();

    /**
     * @brief Release connection back to pool
     * @param conn Connection to release
     */
    void releaseConnection(std::unique_ptr<pqxx::connection> conn);

    /**
     * @brief Initialize pool with initial connections
     */
    void initializePool();

    std::shared_ptr<config::DatabaseConfig> m_config;
    std::queue<std::unique_ptr<pqxx::connection>> m_availableConnections;
    mutable std::mutex m_mutex;
    size_t m_maxPoolSize;
    size_t m_currentSize{0};
    bool m_isShutdown{false};
};

} // namespace sms::database
