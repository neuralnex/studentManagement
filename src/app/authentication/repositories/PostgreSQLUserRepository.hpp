#pragma once

#include "IAuthenticationRepository.hpp"
#include "app/authentication/domain/Entities.hpp"
#include "database/ConnectionPool.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace sms::database {
class ConnectionPool;
}

namespace sms::app::authentication::repositories {

using sms::app::authentication::domain::User;

/**
 * @brief PostgreSQL implementation of IUserRepository
 * 
 * All database operations use parameterized queries to prevent SQL injection.
 */
class PostgreSQLUserRepository : public IUserRepository {
public:
    /**
     * @brief Constructor
     * @param pool Database connection pool
     */
    explicit PostgreSQLUserRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    User create(const User& entity) override;
    std::optional<User> read(int64_t id) const override;
    User update(const User& entity) override;
    bool delete_by_id(int64_t id) override;
    std::vector<User> readAll() const override;
    std::vector<User> readPage(int64_t offset, int64_t limit) const override;
    int64_t count() const override;
    bool exists(int64_t id) const override;

    std::optional<User> findByEmail(const std::string& email) const override;
    std::vector<User> findByRole(int64_t roleId) const override;
    std::vector<User> findByStatus(User::Status status) const override;
    bool emailExists(const std::string& email) const override;
    std::vector<User> findLockedAccounts(int attemptThreshold = 5) const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;

    /**
     * @brief Map database row to User entity
     * @param row Database row reference
     * @return User entity
     */
    static User mapRowToUser(const pqxx::row_ref& row);

    /**
     * @brief Convert status enum to string
     * @param status Status enum
     * @return String representation
     */
    static std::string statusToString(User::Status status);

    /**
     * @brief Convert string to status enum
     * @param statusStr String representation
     * @return Status enum
     */
    static User::Status stringToStatus(const std::string& statusStr);
};

} // namespace sms::app::authentication::repositories
