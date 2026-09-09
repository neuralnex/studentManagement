#pragma once

#include "IAuthenticationRepository.hpp"
#include "database/ConnectionPool.hpp"
#include <memory>

namespace sms::app::authentication::repositories {

/**
 * @brief PostgreSQL implementation of IPermissionRepository
 */
class PostgreSQLPermissionRepository : public IPermissionRepository {
public:
    explicit PostgreSQLPermissionRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    Permission create(const Permission& entity) override;
    std::optional<Permission> read(int64_t id) const override;
    Permission update(const Permission& entity) override;
    bool delete_by_id(int64_t id) override;
    std::vector<Permission> readAll() const override;
    std::vector<Permission> readPage(int64_t offset, int64_t limit) const override;
    int64_t count() const override;
    bool exists(int64_t id) const override;

    std::optional<Permission> findByName(const std::string& name) const override;
    std::vector<Permission> getPermissionsByRole(int64_t roleId) const override;
    bool userHasPermission(int64_t userId, const std::string& permissionName) const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;

    static Permission mapRowToPermission(const pqxx::row_ref& row);
};

} // namespace sms::app::authentication::repositories
