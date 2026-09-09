#pragma once

#include "IAuthenticationService.hpp"
#include "database/ConnectionPool.hpp"
#include "utilities/PasswordHasher.hpp"
#include <memory>
#include <map>
#include <mutex>

namespace sms::app::authentication::services {

/**
 * @brief PostgreSQL-backed authentication service implementation
 */
class AuthenticationService : public IAuthenticationService {
public:
    /**
     * @brief Constructor
     * @param userRepo User repository
     * @param roleRepo Role repository
     * @param permissionRepo Permission repository
     */
    AuthenticationService(
        std::shared_ptr<IUserRepository> userRepo,
        std::shared_ptr<IRoleRepository> roleRepo,
        std::shared_ptr<IPermissionRepository> permRepo
    );

    User registerUser(
        const std::string& email,
        const std::string& password,
        const std::string& firstName,
        const std::string& lastName,
        const std::string& roleName
    ) override;

    LoginResult login(
        const std::string& email,
        const std::string& password,
        const std::string& ipAddress = "",
        const std::string& userAgent = ""
    ) override;

    std::optional<User> validateSession(const std::string& token) const override;

    bool logout(const std::string& token) override;

    bool hasPermission(int64_t userId, const std::string& permissionName) const override;

    void changePassword(
        int64_t userId,
        const std::string& oldPassword,
        const std::string& newPassword
    ) override;

    std::string resetPassword(const std::string& email) override;

    void confirmPasswordReset(
        const std::string& email,
        const std::string& token,
        const std::string& newPassword
    ) override;

    void lockAccount(int64_t userId) override;

    void unlockAccount(int64_t userId) override;

    std::optional<User> getUserProfile(int64_t userId) const override;

    User updateProfile(const User& user) override;

    std::vector<std::string> getActiveSessions(int64_t userId) const override;

private:
    std::shared_ptr<IUserRepository> m_userRepo;
    std::shared_ptr<IRoleRepository> m_roleRepo;
    std::shared_ptr<IPermissionRepository> m_permRepo;

    // In-memory session cache (in production, use Redis or database)
    mutable std::map<std::string, int64_t> m_sessions; // token -> userId
    mutable std::mutex m_sessionMutex;

    // Password reset tokens (in production, use database with expiration)
    std::map<std::string, std::pair<std::string, std::chrono::system_clock::time_point>> 
        m_resetTokens; // token -> (email, expiry)
    std::mutex m_resetMutex;

    static constexpr int MAX_LOGIN_ATTEMPTS = 5;
    static constexpr std::chrono::seconds SESSION_DURATION{3600}; // 1 hour
    static constexpr std::chrono::seconds RESET_TOKEN_DURATION{1800}; // 30 minutes

    /**
     * @brief Create session for user
     * @param userId User ID
     * @param ipAddress Client IP
     * @param userAgent Client user agent
     * @return Session token
     */
    std::string createSession(
        int64_t userId,
        const std::string& ipAddress = "",
        const std::string& userAgent = ""
    );

    /**
     * @brief Check if password reset token is valid
     * @param token Token to check
     * @return Email if valid, std::nullopt otherwise
     */
    std::optional<std::string> validateResetToken(const std::string& token);

    /**
     * @brief Invalidate all sessions for user
     * @param userId User ID
     */
    void invalidateUserSessions(int64_t userId);
};

} // namespace sms::app::authentication::services
