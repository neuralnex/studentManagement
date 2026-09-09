#include "PostgreSQLPermissionRepository.hpp"
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>

namespace sms::app::authentication::repositories {

PostgreSQLPermissionRepository::PostgreSQLPermissionRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(std::move(pool)) {
    if (!m_pool) {
        throw std::invalid_argument("Connection pool cannot be null");
    }
}

Permission PostgreSQLPermissionRepository::mapRowToPermission(const pqxx::row_ref& row) {
    return Permission(
        row["id"].as<int64_t>(),
        row["name"].as<std::string>(),
        row["description"].as<std::string>("")
    );
}

Permission PostgreSQLPermissionRepository::create(const Permission& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                INSERT INTO permissions (name, description, created_at, updated_at)
                VALUES ($1, $2, NOW(), NOW())
                RETURNING id, name, description
            )",
            pqxx::params{entity.name(), entity.description()}
        );

        txn.commit();

        Permission created = mapRowToPermission(result[0]);
        spdlog::info("Permission created: {} (id={})", entity.name(), created.id());
        return created;

    } catch (const pqxx::unique_violation& e) {
        spdlog::error("Permission name already exists: {}", entity.name());
        throw std::runtime_error("Permission name already exists");
    } catch (const std::exception& e) {
        spdlog::error("Failed to create permission: {}", e.what());
        throw std::runtime_error(std::string("Failed to create permission: ") + e.what());
    }
}

std::optional<Permission> PostgreSQLPermissionRepository::read(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, name, description FROM permissions WHERE id = $1",
            pqxx::params{id}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToPermission(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to read permission: {}", e.what());
        throw std::runtime_error(std::string("Failed to read permission: ") + e.what());
    }
}

Permission PostgreSQLPermissionRepository::update(const Permission& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                UPDATE permissions
                SET name = $2, description = $3, updated_at = NOW()
                WHERE id = $1
                RETURNING id, name, description
            )",
            pqxx::params{
                entity.id(),
                entity.name(),
                entity.description()
            }
        );

        if (result.empty()) {
            throw std::runtime_error("Permission not found");
        }

        txn.commit();

        spdlog::info("Permission updated: {} (id={})", entity.name(), entity.id());
        return mapRowToPermission(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to update permission: {}", e.what());
        throw std::runtime_error(std::string("Failed to update permission: ") + e.what());
    }
}

bool PostgreSQLPermissionRepository::delete_by_id(int64_t id) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "DELETE FROM permissions WHERE id = $1",
            pqxx::params{id}
        );

        txn.commit();

        bool deleted = result.affected_rows() > 0;
        if (deleted) {
            spdlog::info("Permission deleted: (id={})", id);
        }
        return deleted;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete permission: {}", e.what());
        throw std::runtime_error(std::string("Failed to delete permission: ") + e.what());
    }
}

std::vector<Permission> PostgreSQLPermissionRepository::readAll() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, name, description FROM permissions ORDER BY name"
        );

        std::vector<Permission> permissions;
        for (auto row : result) {
            permissions.push_back(mapRowToPermission(row));
        }

        return permissions;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read all permissions: {}", e.what());
        throw std::runtime_error(std::string("Failed to read permissions: ") + e.what());
    }
}

std::vector<Permission> PostgreSQLPermissionRepository::readPage(int64_t offset, int64_t limit) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, name, description FROM permissions
                ORDER BY name
                LIMIT $1 OFFSET $2
            )",
            pqxx::params{limit, offset}
        );

        std::vector<Permission> permissions;
        for (auto row : result) {
            permissions.push_back(mapRowToPermission(row));
        }

        return permissions;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read permission page: {}", e.what());
        throw std::runtime_error(std::string("Failed to read permissions: ") + e.what());
    }
}

int64_t PostgreSQLPermissionRepository::count() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec("SELECT COUNT(*) as count FROM permissions");

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count permissions: {}", e.what());
        throw std::runtime_error(std::string("Failed to count permissions: ") + e.what());
    }
}

bool PostgreSQLPermissionRepository::exists(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT 1 FROM permissions WHERE id = $1",
            pqxx::params{id}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check if permission exists: {}", e.what());
        throw std::runtime_error(std::string("Failed to check permission existence: ") + e.what());
    }
}

std::optional<Permission> PostgreSQLPermissionRepository::findByName(const std::string& name) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, name, description FROM permissions WHERE name = $1",
            pqxx::params{name}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToPermission(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to find permission by name: {}", e.what());
        throw std::runtime_error(std::string("Failed to find permission: ") + e.what());
    }
}

std::vector<Permission> PostgreSQLPermissionRepository::getPermissionsByRole(int64_t roleId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT p.id, p.name, p.description FROM permissions p
                INNER JOIN role_permissions rp ON p.id = rp.permission_id
                WHERE rp.role_id = $1
                ORDER BY p.name
            )",
            pqxx::params{roleId}
        );

        std::vector<Permission> permissions;
        for (auto row : result) {
            permissions.push_back(mapRowToPermission(row));
        }

        return permissions;

    } catch (const std::exception& e) {
        spdlog::error("Failed to get permissions by role: {}", e.what());
        throw std::runtime_error(std::string("Failed to get permissions: ") + e.what());
    }
}

bool PostgreSQLPermissionRepository::userHasPermission(int64_t userId, const std::string& permissionName) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT 1 FROM permissions p
                INNER JOIN role_permissions rp ON p.id = rp.permission_id
                INNER JOIN roles r ON rp.role_id = r.id
                INNER JOIN users u ON u.role_id = r.id
                WHERE u.id = $1 AND p.name = $2 AND u.status != 'DELETED' AND r.status = 'ACTIVE'
            )",
            pqxx::params{userId, permissionName}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check user permission: {}", e.what());
        throw std::runtime_error(std::string("Failed to check permission: ") + e.what());
    }
}

} // namespace sms::app::authentication::repositories
