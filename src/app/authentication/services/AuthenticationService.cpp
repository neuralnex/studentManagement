#include "AuthenticationService.hpp"
#include "utilities/PasswordHasher.hpp"
#include <spdlog/spdlog.h>
#include <chrono>

namespace sms::app::authentication::services {

using namespace sms::utilities;
using TimePoint = std::chrono::system_clock::time_point;

AuthenticationService::AuthenticationService(
    std::shared_ptr<IUserRepository> userRepo,
    std::shared_ptr<IRoleRepository> roleRepo,
    std::shared_ptr<IPermissionRepository> permRepo
)
    : m_userRepo(std::move(userRepo)),
      m_roleRepo(std::move(roleRepo)),
      m_permRepo(std::move(permRepo)) {
    
    if (!m_userRepo || !m_roleRepo || !m_permRepo) {
        throw std::invalid_argument("Repositories cannot be null");
    }
}

User AuthenticationService::registerUser(
    const std::string& email,
    const std::string& password,
    const std::string& firstName,
    const std::string& lastName,
    const std::string& roleName
) {
    // Validate input
    if (email.empty() || password.empty()) {
        throw std::runtime_error("Email and password cannot be empty");
    }

    if (!PasswordHasher::isStrong(password)) {
        throw std::runtime_error(
            "Password must be at least 8 characters with uppercase, lowercase, digit, and special character"
        );
    }

    // Check if email already exists
    if (m_userRepo->emailExists(email)) {
        spdlog::warn("Registration attempt with existing email: {}", email);
        throw std::runtime_error("Email already registered");
    }

    // Get role
    auto role = m_roleRepo->findByName(roleName);
    if (!role) {
        spdlog::error("Role not found: {}", roleName);
        throw std::runtime_error("Role not found: " + roleName);
    }

    // Hash password
    std::string passwordHash = PasswordHasher::hash(password);

    // Create user
    User newUser(0, email, passwordHash, role->id(), User::Status::ACTIVE);
    newUser.setFirstName(firstName);
    newUser.setLastName(lastName);

    User createdUser = m_userRepo->create(newUser);
    spdlog::info("User registered: {} (id={})", email, createdUser.id());

    return createdUser;
}

std::string AuthenticationService::createSession(
    int64_t userId,
    const std::string& ipAddress,
    [[maybe_unused]] const std::string& userAgent
) {
    std::string token = PasswordHasher::generateRandomString(64);

    {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        m_sessions[token] = userId;
    }

    spdlog::debug("Session created for user: {} (ip: {})", userId, ipAddress);
    return token;
}

LoginResult AuthenticationService::login(
    const std::string& email,
    const std::string& password,
    const std::string& ipAddress,
    const std::string& userAgent
) {
    // Find user by email
    auto user = m_userRepo->findByEmail(email);
    if (!user) {
        spdlog::warn("Login attempt with non-existent email: {}", email);
        throw std::runtime_error("Invalid email or password");
    }

    // Check if account is locked
    if (user->isLocked()) {
        spdlog::warn("Login attempt to locked account: {} (id={})", email, user->id());
        throw std::runtime_error("Account is locked due to too many failed login attempts");
    }

    // Check if account is active
    if (!user->isActive()) {
        spdlog::warn("Login attempt to inactive account: {} (id={})", email, user->id());
        throw std::runtime_error("Account is not active");
    }

    // Verify password
    if (!PasswordHasher::verify(password, user->passwordHash())) {
        user->incrementFailedAttempts();
        m_userRepo->update(*user);

        int attempts = user->failedLoginAttempts();
        if (attempts >= MAX_LOGIN_ATTEMPTS) {
            user->setStatus(User::Status::LOCKED);
            m_userRepo->update(*user);
            spdlog::warn("Account locked after {} failed attempts: {}", attempts, email);
            throw std::runtime_error("Account locked after too many failed login attempts");
        }

        spdlog::warn("Failed login attempt for {}: {}/{} attempts", 
            email, attempts, MAX_LOGIN_ATTEMPTS);
        throw std::runtime_error("Invalid email or password");
    }

    // Reset failed attempts on successful login
    user->resetFailedAttempts();
    user->updateLastLogin();
    m_userRepo->update(*user);

    // Create session
    std::string sessionToken = createSession(user->id(), ipAddress, userAgent);

    spdlog::info("User logged in: {} (id={})", email, user->id());

    TimePoint expiresAt = std::chrono::system_clock::now() + SESSION_DURATION;

    return LoginResult{
        .user = *user,
        .sessionToken = sessionToken,
        .expiresAt = expiresAt
    };
}

std::optional<User> AuthenticationService::validateSession(const std::string& token) const {
    if (token.empty()) {
        return std::nullopt;
    }

    {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        auto it = m_sessions.find(token);
        if (it == m_sessions.end()) {
            return std::nullopt;
        }

        int64_t userId = it->second;
        return m_userRepo->read(userId);
    }
}

bool AuthenticationService::logout(const std::string& token) {
    {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        auto it = m_sessions.find(token);
        if (it == m_sessions.end()) {
            return false;
        }

        int64_t userId = it->second;
        m_sessions.erase(it);
        spdlog::info("User logged out (id={})", userId);
        return true;
    }
}

bool AuthenticationService::hasPermission(int64_t userId, const std::string& permissionName) const {
    try {
        return m_permRepo->userHasPermission(userId, permissionName);
    } catch (const std::exception& e) {
        spdlog::error("Permission check failed for user {}: {}", userId, e.what());
        return false;
    }
}

void AuthenticationService::changePassword(
    int64_t userId,
    const std::string& oldPassword,
    const std::string& newPassword
) {
    auto user = m_userRepo->read(userId);
    if (!user) {
        throw std::runtime_error("User not found");
    }

    // Verify old password
    if (!PasswordHasher::verify(oldPassword, user->passwordHash())) {
        spdlog::warn("Invalid password change attempt for user: {}", userId);
        throw std::runtime_error("Current password is incorrect");
    }

    // Validate new password
    if (!PasswordHasher::isStrong(newPassword)) {
        throw std::runtime_error(
            "New password must be at least 8 characters with uppercase, lowercase, digit, and special character"
        );
    }

    // Update password
    std::string newHash = PasswordHasher::hash(newPassword);
    user->setPasswordHash(newHash);
    m_userRepo->update(*user);

    // Invalidate all sessions for security
    invalidateUserSessions(userId);

    spdlog::info("Password changed for user: {}", userId);
}

std::string AuthenticationService::resetPassword(const std::string& email) {
    auto user = m_userRepo->findByEmail(email);
    if (!user) {
        // Don't reveal if email exists
        spdlog::info("Password reset requested for non-existent email: {}", email);
        return PasswordHasher::generateRandomString(32);
    }

    std::string resetToken = PasswordHasher::generateRandomString(32);
    TimePoint expiresAt = std::chrono::system_clock::now() + RESET_TOKEN_DURATION;

    {
        std::lock_guard<std::mutex> lock(m_resetMutex);
        m_resetTokens[resetToken] = {email, expiresAt};
    }

    spdlog::info("Password reset token generated for user: {}", user->id());
    return resetToken;
}

std::optional<std::string> AuthenticationService::validateResetToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(m_resetMutex);

    auto it = m_resetTokens.find(token);
    if (it == m_resetTokens.end()) {
        return std::nullopt;
    }

    auto [email, expiresAt] = it->second;

    // Check if token has expired
    if (std::chrono::system_clock::now() > expiresAt) {
        m_resetTokens.erase(it);
        return std::nullopt;
    }

    return email;
}

void AuthenticationService::confirmPasswordReset(
    const std::string& email,
    const std::string& token,
    const std::string& newPassword
) {
    // Validate reset token
    auto validatedEmail = validateResetToken(token);
    if (!validatedEmail || *validatedEmail != email) {
        spdlog::warn("Invalid password reset token for email: {}", email);
        throw std::runtime_error("Invalid or expired reset token");
    }

    // Get user
    auto user = m_userRepo->findByEmail(email);
    if (!user) {
        throw std::runtime_error("User not found");
    }

    // Validate new password
    if (!PasswordHasher::isStrong(newPassword)) {
        throw std::runtime_error(
            "Password must be at least 8 characters with uppercase, lowercase, digit, and special character"
        );
    }

    // Update password
    std::string newHash = PasswordHasher::hash(newPassword);
    user->setPasswordHash(newHash);
    m_userRepo->update(*user);

    // Remove token
    {
        std::lock_guard<std::mutex> lock(m_resetMutex);
        m_resetTokens.erase(token);
    }

    // Invalidate all sessions
    invalidateUserSessions(user->id());

    spdlog::info("Password reset completed for user: {}", user->id());
}

void AuthenticationService::lockAccount(int64_t userId) {
    auto user = m_userRepo->read(userId);
    if (!user) {
        throw std::runtime_error("User not found");
    }

    user->setStatus(User::Status::LOCKED);
    m_userRepo->update(*user);

    invalidateUserSessions(userId);

    spdlog::warn("Account locked: {}", userId);
}

void AuthenticationService::unlockAccount(int64_t userId) {
    auto user = m_userRepo->read(userId);
    if (!user) {
        throw std::runtime_error("User not found");
    }

    user->setStatus(User::Status::ACTIVE);
    user->resetFailedAttempts();
    m_userRepo->update(*user);

    spdlog::info("Account unlocked: {}", userId);
}

std::optional<User> AuthenticationService::getUserProfile(int64_t userId) const {
    auto user = m_userRepo->read(userId);
    if (user) {
        auto roleWithPerms = m_roleRepo->getRoleWithPermissions(user->roleId());
        if (roleWithPerms) {
            user->setRole(*roleWithPerms);
        }
    }
    return user;
}

User AuthenticationService::updateProfile(const User& user) {
    return m_userRepo->update(user);
}

std::vector<std::string> AuthenticationService::getActiveSessions(int64_t userId) const {
    std::vector<std::string> sessions;
    {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        for (const auto& [token, uId] : m_sessions) {
            if (uId == userId) {
                sessions.push_back(token);
            }
        }
    }
    return sessions;
}

void AuthenticationService::invalidateUserSessions(int64_t userId) {
    std::lock_guard<std::mutex> lock(m_sessionMutex);

    std::vector<std::string> tokensToRemove;
    for (const auto& [token, uId] : m_sessions) {
        if (uId == userId) {
            tokensToRemove.push_back(token);
        }
    }

    for (const auto& token : tokensToRemove) {
        m_sessions.erase(token);
    }

    if (!tokensToRemove.empty()) {
        spdlog::debug("Invalidated {} sessions for user: {}", tokensToRemove.size(), userId);
    }
}

} // namespace sms::app::authentication::services
