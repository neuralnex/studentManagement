#include "PostgreSQLUserRepository.hpp"
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>

namespace sms::app::authentication::repositories {

PostgreSQLUserRepository::PostgreSQLUserRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(std::move(pool)) {
    if (!m_pool) {
        throw std::invalid_argument("Connection pool cannot be null");
    }
}

std::string PostgreSQLUserRepository::statusToString(User::Status status) {
    switch (status) {
        case User::Status::ACTIVE: return "ACTIVE";
        case User::Status::INACTIVE: return "INACTIVE";
        case User::Status::LOCKED: return "LOCKED";
        case User::Status::DELETED: return "DELETED";
    }
    return "ACTIVE";
}

User::Status PostgreSQLUserRepository::stringToStatus(const std::string& statusStr) {
    if (statusStr == "ACTIVE") return User::Status::ACTIVE;
    if (statusStr == "INACTIVE") return User::Status::INACTIVE;
    if (statusStr == "LOCKED") return User::Status::LOCKED;
    if (statusStr == "DELETED") return User::Status::DELETED;
    return User::Status::ACTIVE;
}

User PostgreSQLUserRepository::mapRowToUser(const pqxx::row_ref& row) {
    User user;
    user.setId(row["id"].as<int64_t>());
    user.setEmail(row["email"].as<std::string>());
    user.setPasswordHash(row["password_hash"].as<std::string>());
    user.setFirstName(row["first_name"].as<std::string>(""));
    user.setLastName(row["last_name"].as<std::string>(""));
    user.setRoleId(row["role_id"].as<int64_t>());
    user.setStatus(stringToStatus(row["status"].as<std::string>()));

    return user;
}

User PostgreSQLUserRepository::create(const User& entity) {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        // Parameterized query prevents SQL injection
        auto result = txn.exec(
            R"(
                INSERT INTO users
                (email, password_hash, first_name, last_name, role_id, status, created_at, updated_at)
                VALUES ($1, $2, $3, $4, $5, $6, NOW(), NOW())
                RETURNING id, email, password_hash, first_name, last_name, role_id, status, created_at, updated_at
            )",
            pqxx::params{
                entity.email(),
                entity.passwordHash(),
                entity.firstName(),
                entity.lastName(),
                entity.roleId(),
                statusToString(entity.status())
            }
        );

        txn.commit();

        User created = mapRowToUser(result[0]);
        spdlog::info("User created: {} (id={})", entity.email(), created.id());
        return created;

    } catch (const pqxx::unique_violation& e) {
        spdlog::error("Email already exists: {}", entity.email());
        throw std::runtime_error("Email already exists: " + entity.email());
    } catch (const std::exception& e) {
        spdlog::error("Failed to create user: {}", e.what());
        throw std::runtime_error(std::string("Failed to create user: ") + e.what());
    }
}

std::optional<User> PostgreSQLUserRepository::read(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, email, password_hash, first_name, last_name, role_id, status,
                       created_at, updated_at, failed_login_attempts, last_login_at
                FROM users
                WHERE id = $1 AND status != 'DELETED'
            )",
            pqxx::params{id}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToUser(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to read user {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to read user: ") + e.what());
    }
}

User PostgreSQLUserRepository::update(const User& entity) {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                UPDATE users
                SET email = $2, password_hash = $3, first_name = $4, last_name = $5,
                    role_id = $6, status = $7, updated_at = NOW()
                WHERE id = $1
                RETURNING id, email, password_hash, first_name, last_name, role_id, status
            )",
            pqxx::params{
                entity.id(),
                entity.email(),
                entity.passwordHash(),
                entity.firstName(),
                entity.lastName(),
                entity.roleId(),
                statusToString(entity.status())
            }
        );

        if (result.empty()) {
            throw std::runtime_error("User not found");
        }

        txn.commit();

        spdlog::info("User updated: {} (id={})", entity.email(), entity.id());
        return mapRowToUser(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to update user {}: {}", entity.id(), e.what());
        throw std::runtime_error(std::string("Failed to update user: ") + e.what());
    }
}

bool PostgreSQLUserRepository::delete_by_id(int64_t id) {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        // Soft delete - mark as DELETED instead of removing
        auto result = txn.exec(
            R"(
                UPDATE users
                SET status = 'DELETED', updated_at = NOW()
                WHERE id = $1
            )",
            pqxx::params{id}
        );

        txn.commit();

        bool deleted = result.affected_rows() > 0;
        if (deleted) {
            spdlog::info("User deleted: (id={})", id);
        }
        return deleted;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete user {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to delete user: ") + e.what());
    }
}

std::vector<User> PostgreSQLUserRepository::readAll() const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, email, password_hash, first_name, last_name, role_id, status,
                       created_at, updated_at, failed_login_attempts, last_login_at
                FROM users
                WHERE status != 'DELETED'
                ORDER BY created_at DESC
            )"
        );

        std::vector<User> users;
        users.reserve(result.size());
        for (const auto& row : result) {
            users.push_back(mapRowToUser(row));
        }

        return users;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read all users: {}", e.what());
        throw std::runtime_error(std::string("Failed to read users: ") + e.what());
    }
}

std::vector<User> PostgreSQLUserRepository::readPage(int64_t offset, int64_t limit) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, email, password_hash, first_name, last_name, role_id, status,
                       created_at, updated_at, failed_login_attempts, last_login_at
                FROM users
                WHERE status != 'DELETED'
                ORDER BY created_at DESC
                LIMIT $1 OFFSET $2
            )",
            pqxx::params{limit, offset}
        );

        std::vector<User> users;
        users.reserve(result.size());
        for (const auto& row : result) {
            users.push_back(mapRowToUser(row));
        }

        return users;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read user page: {}", e.what());
        throw std::runtime_error(std::string("Failed to read users: ") + e.what());
    }
}

int64_t PostgreSQLUserRepository::count() const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT COUNT(*) as count FROM users WHERE status != 'DELETED'"
        );

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count users: {}", e.what());
        throw std::runtime_error(std::string("Failed to count users: ") + e.what());
    }
}

bool PostgreSQLUserRepository::exists(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT 1 FROM users WHERE id = $1 AND status != 'DELETED'",
            pqxx::params{id}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check if user exists: {}", e.what());
        throw std::runtime_error(std::string("Failed to check user existence: ") + e.what());
    }
}

std::optional<User> PostgreSQLUserRepository::findByEmail(const std::string& email) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, email, password_hash, first_name, last_name, role_id, status,
                       created_at, updated_at, failed_login_attempts, last_login_at
                FROM users
                WHERE email = $1 AND status != 'DELETED'
            )",
            pqxx::params{email}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToUser(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to find user by email: {}", e.what());
        throw std::runtime_error(std::string("Failed to find user: ") + e.what());
    }
}

std::vector<User> PostgreSQLUserRepository::findByRole(int64_t roleId) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, email, password_hash, first_name, last_name, role_id, status,
                       created_at, updated_at, failed_login_attempts, last_login_at
                FROM users
                WHERE role_id = $1 AND status != 'DELETED'
                ORDER BY created_at DESC
            )",
            pqxx::params{roleId}
        );

        std::vector<User> users;
        users.reserve(result.size());
        for (const auto& row : result) {
            users.push_back(mapRowToUser(row));
        }

        return users;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find users by role: {}", e.what());
        throw std::runtime_error(std::string("Failed to find users: ") + e.what());
    }
}

std::vector<User> PostgreSQLUserRepository::findByStatus(User::Status status) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, email, password_hash, first_name, last_name, role_id, status,
                       created_at, updated_at, failed_login_attempts, last_login_at
                FROM users
                WHERE status = $1
                ORDER BY created_at DESC
            )",
            pqxx::params{statusToString(status)}
        );

        std::vector<User> users;
        users.reserve(result.size());
        for (const auto& row : result) {
            users.push_back(mapRowToUser(row));
        }

        return users;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find users by status: {}", e.what());
        throw std::runtime_error(std::string("Failed to find users: ") + e.what());
    }
}

bool PostgreSQLUserRepository::emailExists(const std::string& email) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT 1 FROM users WHERE email = $1 AND status != 'DELETED'",
            pqxx::params{email}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check email existence: {}", e.what());
        throw std::runtime_error(std::string("Failed to check email: ") + e.what());
    }
}

std::vector<User> PostgreSQLUserRepository::findLockedAccounts(int attemptThreshold) const {
    try {
        auto conn = m_pool->acquireConnection();

        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, email, password_hash, first_name, last_name, role_id, status,
                       created_at, updated_at, failed_login_attempts, last_login_at
                FROM users
                WHERE (failed_login_attempts >= $1 OR status = 'LOCKED')
                AND status != 'DELETED'
                ORDER BY last_login_at DESC
            )",
            pqxx::params{attemptThreshold}
        );

        std::vector<User> users;
        users.reserve(result.size());
        for (const auto& row : result) {
            users.push_back(mapRowToUser(row));
        }

        return users;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find locked accounts: {}", e.what());
        throw std::runtime_error(std::string("Failed to find locked accounts: ") + e.what());
    }
}

} // namespace sms::app::authentication::repositories
