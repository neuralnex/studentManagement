#pragma once

#include "../domain/Entities.hpp"
#include "../repositories/IAuthenticationRepository.hpp"
#include <optional>
#include <memory>

namespace sms::app::authentication::services {

using namespace sms::app::authentication::domain;
using namespace sms::app::authentication::repositories;

/**
 * @brief Login result containing user and session info
 */
struct LoginResult {
    User user;
    std::string sessionToken;
    std::chrono::system_clock::time_point expiresAt;
};

/**
 * @brief Authentication service
 * 
 * Handles user authentication, authorization, and session management.
 * Implements RBAC (Role-Based Access Control).
 */
class IAuthenticationService {
public:
    virtual ~IAuthenticationService() = default;

    /**
     * @brief Register a new user
     * @param email User email
     * @param password Plain-text password
     * @param firstName First name
     * @param lastName Last name
     * @param roleName Role name
     * @return Created user
     * @throws std::runtime_error on failure
     */
    virtual User registerUser(
        const std::string& email,
        const std::string& password,
        const std::string& firstName,
        const std::string& lastName,
        const std::string& roleName
    ) = 0;

    /**
     * @brief Authenticate user and create session
     * @param email User email
     * @param password Plain-text password
     * @param ipAddress Client IP address
     * @param userAgent Client user agent
     * @return Login result with session token
     * @throws std::runtime_error on authentication failure
     */
    virtual LoginResult login(
        const std::string& email,
        const std::string& password,
        const std::string& ipAddress = "",
        const std::string& userAgent = ""
    ) = 0;

    /**
     * @brief Validate session token
     * @param token Session token
     * @return User if token valid, std::nullopt otherwise
     */
    virtual std::optional<User> validateSession(const std::string& token) const = 0;

    /**
     * @brief Logout user and invalidate session
     * @param token Session token
     * @return True if logout successful
     */
    virtual bool logout(const std::string& token) = 0;

    /**
     * @brief Check if user has permission
     * @param userId User ID
     * @param permissionName Permission name
     * @return True if user has permission
     */
    virtual bool hasPermission(int64_t userId, const std::string& permissionName) const = 0;

    /**
     * @brief Change user password
     * @param userId User ID
     * @param oldPassword Current password
     * @param newPassword New password
     * @throws std::runtime_error on failure
     */
    virtual void changePassword(
        int64_t userId,
        const std::string& oldPassword,
        const std::string& newPassword
    ) = 0;

    /**
     * @brief Reset user password
     * @param email User email
     * @return Reset token
     * @throws std::runtime_error if user not found
     */
    virtual std::string resetPassword(const std::string& email) = 0;

    /**
     * @brief Confirm password reset with token
     * @param email User email
     * @param token Reset token
     * @param newPassword New password
     * @throws std::runtime_error on failure
     */
    virtual void confirmPasswordReset(
        const std::string& email,
        const std::string& token,
        const std::string& newPassword
    ) = 0;

    /**
     * @brief Lock user account
     * @param userId User ID
     * @throws std::runtime_error on failure
     */
    virtual void lockAccount(int64_t userId) = 0;

    /**
     * @brief Unlock user account
     * @param userId User ID
     * @throws std::runtime_error on failure
     */
    virtual void unlockAccount(int64_t userId) = 0;

    /**
     * @brief Get user profile
     * @param userId User ID
     * @return User with role information
     */
    virtual std::optional<User> getUserProfile(int64_t userId) const = 0;

    /**
     * @brief Update user profile
     * @param user User with updated data
     * @return Updated user
     */
    virtual User updateProfile(const User& user) = 0;

    /**
     * @brief Get all active sessions for a user
     * @param userId User ID
     * @return Vector of session info
     */
    virtual std::vector<std::string> getActiveSessions(int64_t userId) const = 0;
};

} // namespace sms::app::authentication::services
