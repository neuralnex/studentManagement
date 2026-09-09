#pragma once

#include <string>
#include <optional>

namespace sms::utilities {

/**
 * @brief Password hashing and verification utility
 * 
 * Uses bcrypt algorithm for secure password hashing with salt.
 * Never stores plain-text passwords.
 */
class PasswordHasher {
public:
    /**
     * @brief Hash a plain-text password
     * @param password Plain-text password
     * @return Hashed password (bcrypt format)
     * @throws std::runtime_error if hashing fails
     */
    static std::string hash(const std::string& password);

    /**
     * @brief Verify password against hash
     * @param password Plain-text password to verify
     * @param hash Previously hashed password
     * @return True if password matches hash
     * @throws std::runtime_error if verification fails
     */
    static bool verify(const std::string& password, const std::string& hash);

    /**
     * @brief Check password strength
     * @param password Password to check
     * @return True if password meets minimum strength requirements
     * 
     * Requirements:
     * - Minimum 8 characters
     * - At least one uppercase letter
     * - At least one lowercase letter
     * - At least one digit
     * - At least one special character
     */
    static bool isStrong(const std::string& password);

    /**
     * @brief Generate random string for tokens/secrets
     * @param length Length of random string
     * @return Cryptographically random string
     */
    static std::string generateRandomString(size_t length = 32);

private:
    // Static utility class
    PasswordHasher() = delete;
    ~PasswordHasher() = delete;

    static constexpr int BCRYPT_COST = 12;
};

} // namespace sms::utilities
