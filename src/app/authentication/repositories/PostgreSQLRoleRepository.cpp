#include "PostgreSQLRoleRepository.hpp"
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>

namespace sms::app::authentication::repositories {

PostgreSQLRoleRepository::PostgreSQLRoleRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(std::move(pool)) {
    if (!m_pool) {
        throw std::invalid_argument("Connection pool cannot be null");
    }
}

std::string PostgreSQLRoleRepository::statusToString(Role::Status status) {
    switch (status) {
        case Role::Status::ACTIVE: return "ACTIVE";
        case Role::Status::INACTIVE: return "INACTIVE";
        case Role::Status::ARCHIVED: return "ARCHIVED";
    }
    return "ACTIVE";
}

Role::Status PostgreSQLRoleRepository::stringToStatus(const std::string& statusStr) {
    if (statusStr == "ACTIVE") return Role::Status::ACTIVE;
    if (statusStr == "INACTIVE") return Role::Status::INACTIVE;
    if (statusStr == "ARCHIVED") return Role::Status::ARCHIVED;
    return Role::Status::ACTIVE;
}

Role PostgreSQLRoleRepository::mapRowToRole(const pqxx::row_ref& row) {
    Role role;
    role.setId(row["id"].as<int64_t>());
    role.setName(row["name"].as<std::string>());
    role.setDescription(row["description"].as<std::string>(""));
    role.setStatus(stringToStatus(row["status"].as<std::string>()));
    return role;
}

Role PostgreSQLRoleRepository::create(const Role& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                INSERT INTO roles (name, description, status, created_at, updated_at)
                VALUES ($1, $2, $3, NOW(), NOW())
                RETURNING id, name, description, status
            )",
            pqxx::params{
                entity.name(),
                entity.description(),
                statusToString(entity.status())
            }
        );

        txn.commit();

        Role created = mapRowToRole(result[0]);
        spdlog::info("Role created: {} (id={})", entity.name(), created.id());
        return created;

    } catch (const pqxx::unique_violation& e) {
        spdlog::error("Role name already exists: {}", entity.name());
        throw std::runtime_error("Role name already exists: " + entity.name());
    } catch (const std::exception& e) {
        spdlog::error("Failed to create role: {}", e.what());
        throw std::runtime_error(std::string("Failed to create role: ") + e.what());
    }
}

std::optional<Role> PostgreSQLRoleRepository::read(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, name, description, status FROM roles WHERE id = $1",
            pqxx::params{id}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToRole(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to read role {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to read role: ") + e.what());
    }
}

Role PostgreSQLRoleRepository::update(const Role& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                UPDATE roles
                SET name = $2, description = $3, status = $4, updated_at = NOW()
                WHERE id = $1
                RETURNING id, name, description, status
            )",
            pqxx::params{
                entity.id(),
                entity.name(),
                entity.description(),
                statusToString(entity.status())
            }
        );

        if (result.empty()) {
            throw std::runtime_error("Role not found");
        }

        txn.commit();

        spdlog::info("Role updated: {} (id={})", entity.name(), entity.id());
        return mapRowToRole(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to update role {}: {}", entity.id(), e.what());
        throw std::runtime_error(std::string("Failed to update role: ") + e.what());
    }
}

bool PostgreSQLRoleRepository::delete_by_id(int64_t id) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "DELETE FROM roles WHERE id = $1",
            pqxx::params{id}
        );

        txn.commit();

        bool deleted = result.affected_rows() > 0;
        if (deleted) {
            spdlog::info("Role deleted: (id={})", id);
        }
        return deleted;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete role {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to delete role: ") + e.what());
    }
}

std::vector<Role> PostgreSQLRoleRepository::readAll() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, name, description, status FROM roles ORDER BY name"
        );

        std::vector<Role> roles;
        for (auto row : result) {
            roles.push_back(mapRowToRole(row));
        }

        return roles;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read all roles: {}", e.what());
        throw std::runtime_error(std::string("Failed to read roles: ") + e.what());
    }
}

std::vector<Role> PostgreSQLRoleRepository::readPage(int64_t offset, int64_t limit) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, name, description, status FROM roles
                ORDER BY name
                LIMIT $1 OFFSET $2
            )",
            pqxx::params{limit, offset}
        );

        std::vector<Role> roles;
        for (auto row : result) {
            roles.push_back(mapRowToRole(row));
        }

        return roles;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read role page: {}", e.what());
        throw std::runtime_error(std::string("Failed to read roles: ") + e.what());
    }
}

int64_t PostgreSQLRoleRepository::count() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec("SELECT COUNT(*) as count FROM roles");

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count roles: {}", e.what());
        throw std::runtime_error(std::string("Failed to count roles: ") + e.what());
    }
}

bool PostgreSQLRoleRepository::exists(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT 1 FROM roles WHERE id = $1",
            pqxx::params{id}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check if role exists: {}", e.what());
        throw std::runtime_error(std::string("Failed to check role existence: ") + e.what());
    }
}

std::optional<Role> PostgreSQLRoleRepository::findByName(const std::string& name) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, name, description, status FROM roles WHERE name = $1",
            pqxx::params{name}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToRole(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to find role by name: {}", e.what());
        throw std::runtime_error(std::string("Failed to find role: ") + e.what());
    }
}

std::optional<Role> PostgreSQLRoleRepository::getRoleWithPermissions(int64_t roleId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        // Get role
        auto roleResult = txn.exec(
            "SELECT id, name, description, status FROM roles WHERE id = $1",
            pqxx::params{roleId}
        );

        if (roleResult.empty()) {
            return std::nullopt;
        }

        Role role = mapRowToRole(roleResult[0]);

        // Get permissions
        auto permResult = txn.exec(
            R"(
                SELECT p.id, p.name, p.description FROM permissions p
                INNER JOIN role_permissions rp ON p.id = rp.permission_id
                WHERE rp.role_id = $1
                ORDER BY p.name
            )",
            pqxx::params{roleId}
        );

        for (auto row : permResult) {
            Permission perm(
                row["id"].as<int64_t>(),
                row["name"].as<std::string>(),
                row["description"].as<std::string>("")
            );
            role.addPermission(perm);
        }

        return role;

    } catch (const std::exception& e) {
        spdlog::error("Failed to get role with permissions: {}", e.what());
        throw std::runtime_error(std::string("Failed to get role: ") + e.what());
    }
}

void PostgreSQLRoleRepository::assignPermission(int64_t roleId, int64_t permissionId) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        txn.exec(
            R"(
                INSERT INTO role_permissions (role_id, permission_id, created_at)
                VALUES ($1, $2, NOW())
            )",
            pqxx::params{roleId, permissionId}
        );

        txn.commit();

        spdlog::info("Permission assigned to role: {} -> {}", roleId, permissionId);

    } catch (const pqxx::unique_violation&) {
        spdlog::warn("Permission already assigned to role: {} -> {}", roleId, permissionId);
        throw std::runtime_error("Permission already assigned to this role");
    } catch (const std::exception& e) {
        spdlog::error("Failed to assign permission: {}", e.what());
        throw std::runtime_error(std::string("Failed to assign permission: ") + e.what());
    }
}

bool PostgreSQLRoleRepository::revokePermission(int64_t roleId, int64_t permissionId) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "DELETE FROM role_permissions WHERE role_id = $1 AND permission_id = $2",
            pqxx::params{roleId, permissionId}
        );

        txn.commit();

        bool revoked = result.affected_rows() > 0;
        if (revoked) {
            spdlog::info("Permission revoked from role: {} -> {}", roleId, permissionId);
        }
        return revoked;

    } catch (const std::exception& e) {
        spdlog::error("Failed to revoke permission: {}", e.what());
        throw std::runtime_error(std::string("Failed to revoke permission: ") + e.what());
    }
}

std::vector<Role> PostgreSQLRoleRepository::getActiveRoles() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, name, description, status FROM roles WHERE status = 'ACTIVE' ORDER BY name"
        );

        std::vector<Role> roles;
        for (auto row : result) {
            roles.push_back(mapRowToRole(row));
        }

        return roles;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read active roles: {}", e.what());
        throw std::runtime_error(std::string("Failed to read roles: ") + e.what());
    }
}

} // namespace sms::app::authentication::repositories
