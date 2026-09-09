#pragma once

#include "../domain/Entities.hpp"
#include "database/repositories/IRepository.hpp"
#include <optional>
#include <vector>

namespace sms::app::authentication::repositories {

using namespace sms::app::authentication::domain;

/**
 * @brief User repository interface
 * 
 * Extends generic repository with user-specific operations
 */
class IUserRepository : public sms::database::repositories::IRepository<User, int64_t> {
public:
    virtual ~IUserRepository() = default;

    /**
     * @brief Find user by email
     * @param email User email
     * @return User if found, std::nullopt otherwise
     */
    virtual std::optional<User> findByEmail(const std::string& email) const = 0;

    /**
     * @brief Find all users with specific role
     * @param roleId Role ID
     * @return Vector of users with that role
     */
    virtual std::vector<User> findByRole(int64_t roleId) const = 0;

    /**
     * @brief Find users by status
     * @param status User status
     * @return Vector of users with that status
     */
    virtual std::vector<User> findByStatus(User::Status status) const = 0;

    /**
     * @brief Check if email exists
     * @param email Email to check
     * @return True if exists
     */
    virtual bool emailExists(const std::string& email) const = 0;

    /**
     * @brief Get users with failed login attempts
     * @param attemptThreshold Minimum number of attempts
     * @return Vector of users who exceeded threshold
     */
    virtual std::vector<User> findLockedAccounts(int attemptThreshold = 5) const = 0;
};

/**
 * @brief Role repository interface
 * 
 * Extends generic repository with role-specific operations
 */
class IRoleRepository : public sms::database::repositories::IRepository<Role, int64_t> {
public:
    virtual ~IRoleRepository() = default;

    /**
     * @brief Find role by name
     * @param name Role name
     * @return Role if found, std::nullopt otherwise
     */
    virtual std::optional<Role> findByName(const std::string& name) const = 0;

    /**
     * @brief Get role with all its permissions
     * @param roleId Role ID
     * @return Role with permissions loaded
     */
    virtual std::optional<Role> getRoleWithPermissions(int64_t roleId) const = 0;

    /**
     * @brief Assign permission to role
     * @param roleId Role ID
     * @param permissionId Permission ID
     * @throws std::runtime_error on database error or duplicate
     */
    virtual void assignPermission(int64_t roleId, int64_t permissionId) = 0;

    /**
     * @brief Revoke permission from role
     * @param roleId Role ID
     * @param permissionId Permission ID
     * @return True if revoked, false if not found
     */
    virtual bool revokePermission(int64_t roleId, int64_t permissionId) = 0;

    /**
     * @brief Get active roles only
     * @return Vector of active roles
     */
    virtual std::vector<Role> getActiveRoles() const = 0;
};

/**
 * @brief Permission repository interface
 */
class IPermissionRepository : public sms::database::repositories::IRepository<Permission, int64_t> {
public:
    virtual ~IPermissionRepository() = default;

    /**
     * @brief Find permission by name
     * @param name Permission name
     * @return Permission if found, std::nullopt otherwise
     */
    virtual std::optional<Permission> findByName(const std::string& name) const = 0;

    /**
     * @brief Get all permissions for a role
     * @param roleId Role ID
     * @return Vector of permissions
     */
    virtual std::vector<Permission> getPermissionsByRole(int64_t roleId) const = 0;

    /**
     * @brief Check if user has specific permission
     * @param userId User ID
     * @param permissionName Permission name
     * @return True if user has permission
     */
    virtual bool userHasPermission(int64_t userId, const std::string& permissionName) const = 0;
};

} // namespace sms::app::authentication::repositories
