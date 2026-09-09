#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace sms::app::authentication::domain {

using json = nlohmann::json;
using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief Permission entity
 * 
 * Represents a specific system permission/action that can be performed.
 * Examples: "create_student", "view_results", "modify_grades"
 */
class Permission {
public:
    Permission() = default;
    
    Permission(int64_t id, const std::string& name, const std::string& description)
        : m_id(id), m_name(name), m_description(description) {}

    [[nodiscard]] int64_t id() const { return m_id; }
    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] const std::string& description() const { return m_description; }

    void setId(int64_t id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }
    void setDescription(const std::string& desc) { m_description = desc; }

    bool operator==(const Permission& other) const {
        return m_id == other.m_id;
    }

private:
    int64_t m_id{0};
    std::string m_name;
    std::string m_description;
};

/**
 * @brief Role entity
 * 
 * Represents a set of permissions. Examples: "Administrator", "Registrar", "Student"
 */
class Role {
public:
    enum class Status {
        ACTIVE,
        INACTIVE,
        ARCHIVED
    };

    Role() = default;
    
    Role(int64_t id, const std::string& name, const std::string& description, Status status = Status::ACTIVE)
        : m_id(id), m_name(name), m_description(description), m_status(status) {}

    [[nodiscard]] int64_t id() const { return m_id; }
    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] const std::string& description() const { return m_description; }
    [[nodiscard]] Status status() const { return m_status; }
    [[nodiscard]] const std::vector<Permission>& permissions() const { return m_permissions; }

    void setId(int64_t id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }
    void setDescription(const std::string& desc) { m_description = desc; }
    void setStatus(Status status) { m_status = status; }

    void addPermission(const Permission& perm) {
        m_permissions.push_back(perm);
    }

    void clearPermissions() {
        m_permissions.clear();
    }

    bool operator==(const Role& other) const {
        return m_id == other.m_id;
    }

private:
    int64_t m_id{0};
    std::string m_name;
    std::string m_description;
    Status m_status{Status::ACTIVE};
    std::vector<Permission> m_permissions;
};

/**
 * @brief User entity
 * 
 * Represents a system user with authentication credentials and role assignment.
 */
class User {
public:
    enum class Status {
        ACTIVE,
        INACTIVE,
        LOCKED,
        DELETED
    };

    User() = default;

    User(int64_t id, const std::string& email, const std::string& passwordHash, 
         int64_t roleId, Status status = Status::ACTIVE)
        : m_id(id), m_email(email), m_passwordHash(passwordHash), 
          m_roleId(roleId), m_status(status) {}

    [[nodiscard]] int64_t id() const { return m_id; }
    [[nodiscard]] const std::string& email() const { return m_email; }
    [[nodiscard]] const std::string& passwordHash() const { return m_passwordHash; }
    [[nodiscard]] int64_t roleId() const { return m_roleId; }
    [[nodiscard]] Status status() const { return m_status; }
    [[nodiscard]] const std::string& firstName() const { return m_firstName; }
    [[nodiscard]] const std::string& lastName() const { return m_lastName; }
    [[nodiscard]] int failedLoginAttempts() const { return m_failedLoginAttempts; }
    [[nodiscard]] const TimePoint& lastLoginAt() const { return m_lastLoginAt; }
    [[nodiscard]] const TimePoint& createdAt() const { return m_createdAt; }
    [[nodiscard]] const TimePoint& updatedAt() const { return m_updatedAt; }
    [[nodiscard]] const Role& role() const { return m_role; }

    void setId(int64_t id) { m_id = id; }
    void setEmail(const std::string& email) { m_email = email; }
    void setPasswordHash(const std::string& hash) { m_passwordHash = hash; }
    void setRoleId(int64_t roleId) { m_roleId = roleId; }
    void setStatus(Status status) { m_status = status; }
    void setFirstName(const std::string& name) { m_firstName = name; }
    void setLastName(const std::string& name) { m_lastName = name; }
    void setRole(const Role& role) { m_role = role; }

    /**
     * @brief Increment failed login attempts
     */
    void incrementFailedAttempts() {
        m_failedLoginAttempts++;
    }

    /**
     * @brief Reset failed login attempts
     */
    void resetFailedAttempts() {
        m_failedLoginAttempts = 0;
    }

    /**
     * @brief Update last login timestamp
     */
    void updateLastLogin() {
        m_lastLoginAt = std::chrono::system_clock::now();
    }

    /**
     * @brief Check if account is locked due to failed attempts
     * @return True if locked
     */
    [[nodiscard]] bool isLocked() const {
        return m_status == Status::LOCKED || m_failedLoginAttempts >= 5;
    }

    /**
     * @brief Check if account is active
     * @return True if active
     */
    [[nodiscard]] bool isActive() const {
        return m_status == Status::ACTIVE;
    }

    bool operator==(const User& other) const {
        return m_id == other.m_id;
    }

private:
    int64_t m_id{0};
    std::string m_email;
    std::string m_passwordHash;
    std::string m_firstName;
    std::string m_lastName;
    int64_t m_roleId{0};
    Role m_role;
    Status m_status{Status::ACTIVE};
    int m_failedLoginAttempts{0};
    TimePoint m_lastLoginAt;
    TimePoint m_createdAt;
    TimePoint m_updatedAt;
};

} // namespace sms::app::authentication::domain
