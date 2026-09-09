#include "PasswordHasher.hpp"
#include <spdlog/spdlog.h>
#include <random>
#include <regex>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>

namespace sms::utilities {

static std::string sha256Hex(const std::string& input) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdLen = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1) {
            EVP_DigestUpdate(ctx, input.data(), input.size());
            EVP_DigestFinal_ex(ctx, md, &mdLen);
        }
        EVP_MD_CTX_free(ctx);
    }

    std::ostringstream ss;
    for (unsigned int i = 0; i < mdLen; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md[i]);
    }
    return ss.str();
}

std::string PasswordHasher::hash(const std::string& password) {
    if (password.empty()) {
        throw std::runtime_error("Password cannot be empty");
    }

    if (password.length() < 8) {
        throw std::runtime_error("Password too short");
    }

    try {
        std::string salt = generateRandomString(22);
        std::string digest = sha256Hex(salt + password);
        std::string fullHash = "$2b$" + std::to_string(BCRYPT_COST) + "$" + salt + "$" + digest;

        spdlog::debug("Password hashed successfully");
        return fullHash;
    } catch (const std::exception& e) {
        spdlog::error("Password hashing failed: {}", e.what());
        throw std::runtime_error("Password hashing failed");
    }
}

bool PasswordHasher::verify(const std::string& password, const std::string& hash) {
    if (password.empty() || hash.empty()) {
        return false;
    }

    try {
        std::string prefix = "$2b$" + std::to_string(BCRYPT_COST) + "$";
        if (hash.rfind(prefix, 0) != 0) {
            return false;
        }

        std::string remainder = hash.substr(prefix.length());
        size_t delimPos = remainder.find('$');
        if (delimPos == std::string::npos) {
            return false;
        }

        std::string salt = remainder.substr(0, delimPos);
        std::string expectedDigest = sha256Hex(salt + password);
        std::string expectedHash = prefix + salt + "$" + expectedDigest;

        if (hash.length() != expectedHash.length()) {
            return false;
        }

        int result = 0;
        for (size_t i = 0; i < hash.length(); ++i) {
            result |= hash[i] ^ expectedHash[i];
        }

        spdlog::debug("Password verification completed");
        return result == 0;
    } catch (const std::exception& e) {
        spdlog::error("Password verification failed: {}", e.what());
        return false;
    }
}

bool PasswordHasher::isStrong(const std::string& password) {
    if (password.length() < 8) {
        return false;
    }

    bool hasUpper = std::any_of(password.begin(), password.end(), 
        [](unsigned char c) { return std::isupper(c); });
    
    bool hasLower = std::any_of(password.begin(), password.end(), 
        [](unsigned char c) { return std::islower(c); });
    
    bool hasDigit = std::any_of(password.begin(), password.end(), 
        [](unsigned char c) { return std::isdigit(c); });
    
    bool hasSpecial = std::any_of(password.begin(), password.end(),
        [](unsigned char c) { 
            return !std::isalnum(c) && !std::isspace(c); 
        });

    return hasUpper && hasLower && hasDigit && hasSpecial;
}

std::string PasswordHasher::generateRandomString(size_t length) {
    static constexpr const char CHARSET[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(CHARSET) - 2);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += CHARSET[dis(gen)];
    }
    
    return result;
}

} // namespace sms::utilities
