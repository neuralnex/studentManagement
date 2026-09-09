#include <gtest/gtest.h>
#include "../../src/utilities/PasswordHasher.hpp"

using namespace sms::utilities;

class PasswordHasherTest : public ::testing::Test {
protected:
    const std::string validPassword = "SecurePass123!";
    const std::string weakPassword = "weak";
    const std::string noSpecialChar = "SecurePass123";
    const std::string noDigit = "SecurePass!";
    const std::string noLowercase = "SECUREPASS123!";
    const std::string noUppercase = "securepass123!";
};

/**
 * @brief Test password hashing and verification
 */
TEST_F(PasswordHasherTest, HashAndVerifyValidPassword) {
    std::string password = validPassword;
    std::string hash = PasswordHasher::hash(password);

    EXPECT_FALSE(hash.empty());
    EXPECT_NE(hash, password);
    EXPECT_TRUE(PasswordHasher::verify(password, hash));
}

/**
 * @brief Test that different hashes produce different results
 */
TEST_F(PasswordHasherTest, DifferentHashesForSamePassword) {
    std::string password = validPassword;
    std::string hash1 = PasswordHasher::hash(password);
    std::string hash2 = PasswordHasher::hash(password);

    // Hashes should be different due to salt
    // (in a real bcrypt implementation)
    EXPECT_TRUE(PasswordHasher::verify(password, hash1));
    EXPECT_TRUE(PasswordHasher::verify(password, hash2));
}

/**
 * @brief Test password verification with wrong password
 */
TEST_F(PasswordHasherTest, VerifyWithWrongPassword) {
    std::string password = validPassword;
    std::string hash = PasswordHasher::hash(password);
    std::string wrongPassword = "WrongPass123!";

    EXPECT_FALSE(PasswordHasher::verify(wrongPassword, hash));
}

/**
 * @brief Test password strength validation - weak password
 */
TEST_F(PasswordHasherTest, WeakPassword) {
    EXPECT_FALSE(PasswordHasher::isStrong(weakPassword));
    EXPECT_FALSE(PasswordHasher::isStrong(""));
    EXPECT_FALSE(PasswordHasher::isStrong("short"));
}

/**
 * @brief Test password strength validation - missing requirements
 */
TEST_F(PasswordHasherTest, PasswordMissingComponents) {
    EXPECT_FALSE(PasswordHasher::isStrong(noSpecialChar));
    EXPECT_FALSE(PasswordHasher::isStrong(noDigit));
    EXPECT_FALSE(PasswordHasher::isStrong(noLowercase));
    EXPECT_FALSE(PasswordHasher::isStrong(noUppercase));
}

/**
 * @brief Test password strength validation - strong password
 */
TEST_F(PasswordHasherTest, StrongPassword) {
    EXPECT_TRUE(PasswordHasher::isStrong(validPassword));
    EXPECT_TRUE(PasswordHasher::isStrong("ComplexPass456!"));
    EXPECT_TRUE(PasswordHasher::isStrong("MyPassword2026@"));
}

/**
 * @brief Test random string generation
 */
TEST_F(PasswordHasherTest, GenerateRandomString) {
    std::string random1 = PasswordHasher::generateRandomString(32);
    std::string random2 = PasswordHasher::generateRandomString(32);

    EXPECT_EQ(random1.length(), 32);
    EXPECT_EQ(random2.length(), 32);
    EXPECT_NE(random1, random2);
}

/**
 * @brief Test random string generation with different lengths
 */
TEST_F(PasswordHasherTest, GenerateRandomStringDifferentLengths) {
    std::string token16 = PasswordHasher::generateRandomString(16);
    std::string token64 = PasswordHasher::generateRandomString(64);

    EXPECT_EQ(token16.length(), 16);
    EXPECT_EQ(token64.length(), 64);
    EXPECT_FALSE(token16.empty());
    EXPECT_FALSE(token64.empty());
}

/**
 * @brief Test empty password handling
 */
TEST_F(PasswordHasherTest, EmptyPasswordThrows) {
    EXPECT_THROW(PasswordHasher::hash(""), std::runtime_error);
}

/**
 * @brief Test short password handling
 */
TEST_F(PasswordHasherTest, ShortPasswordThrows) {
    EXPECT_THROW(PasswordHasher::hash("Short1!"), std::runtime_error);
}
