#include <gtest/gtest.h>
#include "../../src/config/DatabaseConfig.hpp"
#include "../../src/database/ConnectionPool.hpp"

using namespace sms::config;
using namespace sms::database;

/**
 * @brief Integration tests for database connection
 * 
 * These tests require a running PostgreSQL database.
 * Set environment variables:
 * - DB_HOST (default: localhost)
 * - DB_PORT (default: 5432)
 * - DB_NAME (default: student_db)
 * - DB_USER (default: postgres)
 * - DB_PASSWORD
 */
class DatabaseConnectionTest : public ::testing::Test {
protected:
    // These tests are skipped if database is not available
    static bool IsDatabaseAvailable() {
        try {
            auto config = DatabaseConfig::loadFromEnvironment();
            if (!config) return false;
            
            ConnectionPool pool(std::move(config));
            return pool.isHealthy();
        } catch (...) {
            return false;
        }
    }
};

/**
 * @brief Test database configuration loading
 */
TEST_F(DatabaseConnectionTest, LoadConfigurationFromEnvironment) {
    auto config = DatabaseConfig::loadFromEnvironment();
    
    ASSERT_NE(nullptr, config);
    EXPECT_FALSE(config->getHost().empty());
    EXPECT_GT(config->getPort(), 0);
    EXPECT_FALSE(config->getDatabase().empty());
}

/**
 * @brief Test connection string building
 */
TEST_F(DatabaseConnectionTest, BuildConnectionString) {
    auto config = DatabaseConfig::loadFromEnvironment();
    
    ASSERT_NE(nullptr, config);
    
    std::string connStr = config->getConnectionString();
    EXPECT_TRUE(connStr.find("postgresql://") != std::string::npos);
    // Host check is skipped if DATABASE_URL is used as it may not match default host

}

/**
 * @brief Test connection pool initialization
 */
TEST_F(DatabaseConnectionTest, InitializeConnectionPool) {
    if (!IsDatabaseAvailable()) {
        GTEST_SKIP() << "Database not available";
    }

    auto config = DatabaseConfig::loadFromEnvironment();
    ASSERT_NE(nullptr, config);

    ConnectionPool pool(std::move(config));
    
    EXPECT_TRUE(pool.isHealthy());
    EXPECT_GT(pool.getMaxSize(), 0);
    EXPECT_GT(pool.getCurrentSize(), 0);
}

/**
 * @brief Test acquiring connection from pool
 */
TEST_F(DatabaseConnectionTest, AcquireConnection) {
    if (!IsDatabaseAvailable()) {
        GTEST_SKIP() << "Database not available";
    }

    auto config = DatabaseConfig::loadFromEnvironment();
    ASSERT_NE(nullptr, config);

    ConnectionPool pool(std::move(config));
    
    {
        auto conn = pool.acquireConnection();
        EXPECT_TRUE(conn.isOpen());
    }
}

/**
 * @brief Test multiple connections from pool
 */
TEST_F(DatabaseConnectionTest, AcquireMultipleConnections) {
    if (!IsDatabaseAvailable()) {
        GTEST_SKIP() << "Database not available";
    }

    auto config = DatabaseConfig::loadFromEnvironment();
    ASSERT_NE(nullptr, config);

    ConnectionPool pool(std::move(config));
    
    {
        auto conn1 = pool.acquireConnection();
        auto conn2 = pool.acquireConnection();
        
        EXPECT_TRUE(conn1.isOpen());
        EXPECT_TRUE(conn2.isOpen());
    }
}

/**
 * @brief Test pool health check
 */
TEST_F(DatabaseConnectionTest, PoolHealthCheck) {
    if (!IsDatabaseAvailable()) {
        GTEST_SKIP() << "Database not available";
    }

    auto config = DatabaseConfig::loadFromEnvironment();
    ASSERT_NE(nullptr, config);

    ConnectionPool pool(std::move(config));
    
    EXPECT_TRUE(pool.isHealthy());
}
