#include "ConnectionPool.hpp"
#include <spdlog/spdlog.h>

namespace sms::database {

ConnectionPool::ConnectionPool(const std::shared_ptr<config::DatabaseConfig>& config)
    : m_config(config), m_maxPoolSize(config->getPoolSize()) {
    
    if (!m_config) {
        throw std::runtime_error("Database config cannot be null");
    }

    initializePool();
}

ConnectionPool::~ConnectionPool() {
    reset();
}

std::unique_ptr<pqxx::connection> ConnectionPool::createConnection() {
    try {
        auto conn = std::make_unique<pqxx::connection>(m_config->getConnectionString());
        
        if (!conn->is_open()) {
            throw std::runtime_error("Failed to establish database connection");
        }

        spdlog::debug("Database connection established");
        return conn;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create database connection: {}", e.what());
        throw std::runtime_error(
            std::string("Database connection failed: ") + e.what()
        );
    }
}

void ConnectionPool::initializePool() {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        // Create initial minimum connections (1/2 of pool size)
        size_t initialConnections = std::max(size_t(1), m_maxPoolSize / 2);
        
        for (size_t i = 0; i < initialConnections; ++i) {
            m_availableConnections.push(createConnection());
            ++m_currentSize;
        }

        spdlog::info("Connection pool initialized with {} connections", 
            initialConnections);
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize connection pool: {}", e.what());
        throw;
    }
}

ConnectionPool::Connection ConnectionPool::acquireConnection() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isShutdown) {
        throw std::runtime_error("Connection pool has been shut down");
    }

    for (int attempt = 0; attempt < 3; ++attempt) {
        std::unique_ptr<pqxx::connection> conn;

        // Try to get existing connection
        if (!m_availableConnections.empty()) {
            conn = std::move(m_availableConnections.front());
            m_availableConnections.pop();

            // Health check: Verify connection is actually alive
            try {
                pqxx::work txn(*conn);
                txn.exec("SELECT 1");
            } catch (const std::exception& e) {
                spdlog::warn("Connection from pool was dead, discarding: {}", e.what());
                --m_currentSize;
                continue; // Try again
            }
        }
        // Create new connection if pool not at capacity
        else if (m_currentSize < m_maxPoolSize) {
            conn = createConnection();
            ++m_currentSize;
        }
        // All connections in use
        else {
            throw std::runtime_error(
                "No connections available in pool (size: " +
                std::to_string(m_maxPoolSize) + ")"
            );
        }

        // Final verification
        if (conn && conn->is_open()) {
            return Connection(*this, std::move(conn));
        }
    }

    throw std::runtime_error("Failed to acquire a healthy database connection after multiple attempts");
}

void ConnectionPool::releaseConnection(std::unique_ptr<pqxx::connection> conn) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isShutdown || !conn || !conn->is_open()) {
        // Don't return invalid connections
        --m_currentSize;
        return;
    }

    m_availableConnections.push(std::move(conn));
}

size_t ConnectionPool::getCurrentSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSize;
}

bool ConnectionPool::isHealthy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_isShutdown || m_currentSize == 0) {
        return false;
    }

    // Quick check on first available connection
    if (!m_availableConnections.empty()) {
        return m_availableConnections.front()->is_open();
    }

    return true; // Connections may be checked out
}

void ConnectionPool::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_isShutdown = true;

    while (!m_availableConnections.empty()) {
        m_availableConnections.pop();
    }

    m_currentSize = 0;
    spdlog::info("Connection pool reset");
}

} // namespace sms::database
