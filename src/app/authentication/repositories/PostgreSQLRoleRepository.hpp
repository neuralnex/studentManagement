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

using sms::app::authentication::domain::Role;

/**
 * @brief PostgreSQL implementation of IRoleRepository
 */
class PostgreSQLRoleRepository : public IRoleRepository {
public:
    explicit PostgreSQLRoleRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    Role create(const Role& entity) override;
    std::optional<Role> read(int64_t id) const override;
    Role update(const Role& entity) override;
    bool delete_by_id(int64_t id) override;
    std::vector<Role> readAll() const override;
    std::vector<Role> readPage(int64_t offset, int64_t limit) const override;
    int64_t count() const override;
    bool exists(int64_t id) const override;

    std::optional<Role> findByName(const std::string& name) const override;
    std::optional<Role> getRoleWithPermissions(int64_t roleId) const override;
    void assignPermission(int64_t roleId, int64_t permissionId) override;
    bool revokePermission(int64_t roleId, int64_t permissionId) override;
    std::vector<Role> getActiveRoles() const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;

    static Role mapRowToRole(const pqxx::row_ref& row);
    static std::string statusToString(Role::Status status);
    static Role::Status stringToStatus(const std::string& statusStr);
};

} // namespace sms::app::authentication::repositories
