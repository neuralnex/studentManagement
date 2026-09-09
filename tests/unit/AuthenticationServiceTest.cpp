#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/app/authentication/services/AuthenticationService.hpp"

using namespace sms::app::authentication;
using namespace sms::app::authentication::domain;
using namespace sms::app::authentication::services;
using namespace sms::app::authentication::repositories;

/**
 * @brief Mock User Repository
 */
class MockUserRepository : public IUserRepository {
public:
    MOCK_METHOD(User, create, (const User&), (override));
    MOCK_METHOD(std::optional<User>, read, (int64_t), (const, override));
    MOCK_METHOD(User, update, (const User&), (override));
    MOCK_METHOD(bool, delete_by_id, (int64_t), (override));
    MOCK_METHOD(std::vector<User>, readAll, (), (const, override));
    MOCK_METHOD(std::vector<User>, readPage, (int64_t, int64_t), (const, override));
    MOCK_METHOD(int64_t, count, (), (const, override));
    MOCK_METHOD(bool, exists, (int64_t), (const, override));
    MOCK_METHOD(std::optional<User>, findByEmail, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<User>, findByRole, (int64_t), (const, override));
    MOCK_METHOD(std::vector<User>, findByStatus, (User::Status), (const, override));
    MOCK_METHOD(bool, emailExists, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<User>, findLockedAccounts, (int), (const, override));
};

/**
 * @brief Mock Role Repository
 */
class MockRoleRepository : public IRoleRepository {
public:
    MOCK_METHOD(Role, create, (const Role&), (override));
    MOCK_METHOD(std::optional<Role>, read, (int64_t), (const, override));
    MOCK_METHOD(Role, update, (const Role&), (override));
    MOCK_METHOD(bool, delete_by_id, (int64_t), (override));
    MOCK_METHOD(std::vector<Role>, readAll, (), (const, override));
    MOCK_METHOD(std::vector<Role>, readPage, (int64_t, int64_t), (const, override));
    MOCK_METHOD(int64_t, count, (), (const, override));
    MOCK_METHOD(bool, exists, (int64_t), (const, override));
    MOCK_METHOD(std::optional<Role>, findByName, (const std::string&), (const, override));
    MOCK_METHOD(std::optional<Role>, getRoleWithPermissions, (int64_t), (const, override));
    MOCK_METHOD(void, assignPermission, (int64_t, int64_t), (override));
    MOCK_METHOD(bool, revokePermission, (int64_t, int64_t), (override));
    MOCK_METHOD(std::vector<Role>, getActiveRoles, (), (const, override));
};

/**
 * @brief Mock Permission Repository
 */
class MockPermissionRepository : public IPermissionRepository {
public:
    MOCK_METHOD(Permission, create, (const Permission&), (override));
    MOCK_METHOD(std::optional<Permission>, read, (int64_t), (const, override));
    MOCK_METHOD(Permission, update, (const Permission&), (override));
    MOCK_METHOD(bool, delete_by_id, (int64_t), (override));
    MOCK_METHOD(std::vector<Permission>, readAll, (), (const, override));
    MOCK_METHOD(std::vector<Permission>, readPage, (int64_t, int64_t), (const, override));
    MOCK_METHOD(int64_t, count, (), (const, override));
    MOCK_METHOD(bool, exists, (int64_t), (const, override));
    MOCK_METHOD(std::optional<Permission>, findByName, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Permission>, getPermissionsByRole, (int64_t), (const, override));
    MOCK_METHOD(bool, userHasPermission, (int64_t, const std::string&), (const, override));
};

class AuthenticationServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockUserRepository> userRepo;
    std::shared_ptr<MockRoleRepository> roleRepo;
    std::shared_ptr<MockPermissionRepository> permRepo;
    std::shared_ptr<AuthenticationService> authService;

    void SetUp() override {
        userRepo = std::make_shared<MockUserRepository>();
        roleRepo = std::make_shared<MockRoleRepository>();
        permRepo = std::make_shared<MockPermissionRepository>();
        authService = std::make_shared<AuthenticationService>(userRepo, roleRepo, permRepo);
    }

    Role createStudentRole() {
        Role role(1, "Student", "Student role");
        Permission perm(1, "view_results", "Can view results");
        role.addPermission(perm);
        return role;
    }
};

/**
 * @brief Test user registration success
 */
TEST_F(AuthenticationServiceTest, RegisterUserSuccess) {
    Role studentRole = createStudentRole();

    EXPECT_CALL(*roleRepo, findByName("Student"))
        .WillOnce(::testing::Return(studentRole));

    EXPECT_CALL(*userRepo, emailExists("student@test.edu"))
        .WillOnce(::testing::Return(false));

    User createdUser(1, "student@test.edu", "hashed_password", 1, User::Status::ACTIVE);
    createdUser.setFirstName("John");
    createdUser.setLastName("Doe");

    EXPECT_CALL(*userRepo, create(::testing::_))
        .WillOnce(::testing::Return(createdUser));

    User result = authService->registerUser(
        "student@test.edu",
        "SecurePass123!",
        "John",
        "Doe",
        "Student"
    );

    EXPECT_EQ(result.email(), "student@test.edu");
    EXPECT_EQ(result.firstName(), "John");
    EXPECT_EQ(result.lastName(), "Doe");
}

/**
 * @brief Test user registration with existing email
 */
TEST_F(AuthenticationServiceTest, RegisterUserEmailExists) {
    EXPECT_CALL(*userRepo, emailExists("student@test.edu"))
        .WillOnce(::testing::Return(true));

    EXPECT_THROW(
        authService->registerUser(
            "student@test.edu",
            "SecurePass123!",
            "John",
            "Doe",
            "Student"
        ),
        std::runtime_error
    );
}

/**
 * @brief Test user registration with weak password
 */
TEST_F(AuthenticationServiceTest, RegisterUserWeakPassword) {
    EXPECT_THROW(
        authService->registerUser(
            "student@test.edu",
            "weak",
            "John",
            "Doe",
            "Student"
        ),
        std::runtime_error
    );
}

/**
 * @brief Test successful login
 */
TEST_F(AuthenticationServiceTest, LoginSuccess) {
    std::string passwordHash = sms::utilities::PasswordHasher::hash("SecurePass123!");
    User user(1, "student@test.edu", passwordHash, 1, User::Status::ACTIVE);
    user.setFirstName("John");
    user.setLastName("Doe");

    EXPECT_CALL(*userRepo, findByEmail("student@test.edu"))
        .WillOnce(::testing::Return(user));

    EXPECT_CALL(*userRepo, update(::testing::_))
        .WillOnce(::testing::Return(user));

    LoginResult result = authService->login(
        "student@test.edu",
        "SecurePass123!",
        "192.168.1.1",
        "Mozilla"
    );

    EXPECT_EQ(result.user.email(), "student@test.edu");
    EXPECT_FALSE(result.sessionToken.empty());
    EXPECT_GT(result.expiresAt.time_since_epoch().count(), 0);
}

/**
 * @brief Test login with non-existent user
 */
TEST_F(AuthenticationServiceTest, LoginUserNotFound) {
    EXPECT_CALL(*userRepo, findByEmail("nonexistent@test.edu"))
        .WillOnce(::testing::Return(std::nullopt));

    EXPECT_THROW(
        authService->login("nonexistent@test.edu", "SecurePass123!"),
        std::runtime_error
    );
}

/**
 * @brief Test login with inactive account
 */
TEST_F(AuthenticationServiceTest, LoginInactiveAccount) {
    User user(1, "student@test.edu", "", 1, User::Status::INACTIVE);

    EXPECT_CALL(*userRepo, findByEmail("student@test.edu"))
        .WillOnce(::testing::Return(user));

    EXPECT_THROW(
        authService->login("student@test.edu", "SecurePass123!"),
        std::runtime_error
    );
}

/**
 * @brief Test session validation
 */
TEST_F(AuthenticationServiceTest, ValidateSessionSuccess) {
    std::string passwordHash = sms::utilities::PasswordHasher::hash("SecurePass123!");
    User user(1, "student@test.edu", passwordHash, 1, User::Status::ACTIVE);

    EXPECT_CALL(*userRepo, findByEmail("student@test.edu"))
        .WillOnce(::testing::Return(user));

    EXPECT_CALL(*userRepo, update(::testing::_))
        .WillOnce(::testing::Return(user));

    // Login to get session token
    LoginResult result = authService->login("student@test.edu", "SecurePass123!");

    // Now validate the session
    EXPECT_CALL(*userRepo, read(1))
        .WillOnce(::testing::Return(user));

    auto validatedUser = authService->validateSession(result.sessionToken);
    EXPECT_TRUE(validatedUser.has_value());
    EXPECT_EQ(validatedUser->email(), "student@test.edu");
}

/**
 * @brief Test logout
 */
TEST_F(AuthenticationServiceTest, LogoutSuccess) {
    std::string passwordHash = sms::utilities::PasswordHasher::hash("SecurePass123!");
    User user(1, "student@test.edu", passwordHash, 1, User::Status::ACTIVE);

    EXPECT_CALL(*userRepo, findByEmail("student@test.edu"))
        .WillOnce(::testing::Return(user));

    EXPECT_CALL(*userRepo, update(::testing::_))
        .WillOnce(::testing::Return(user));

    // Login
    LoginResult result = authService->login("student@test.edu", "SecurePass123!");

    // Logout
    bool logoutResult = authService->logout(result.sessionToken);
    EXPECT_TRUE(logoutResult);

    // Validate that session is no longer valid
    EXPECT_CALL(*userRepo, read(::testing::_))
        .Times(0);

    auto validatedUser = authService->validateSession(result.sessionToken);
    EXPECT_FALSE(validatedUser.has_value());
}

/**
 * @brief Test permission check
 */
TEST_F(AuthenticationServiceTest, HasPermission) {
    EXPECT_CALL(*permRepo, userHasPermission(1, "view_results"))
        .WillOnce(::testing::Return(true));

    bool hasPermission = authService->hasPermission(1, "view_results");
    EXPECT_TRUE(hasPermission);
}

/**
 * @brief Test password change
 */
TEST_F(AuthenticationServiceTest, ChangePasswordSuccess) {
    std::string oldPasswordHash = sms::utilities::PasswordHasher::hash("OldPass123!");
    User user(1, "student@test.edu", oldPasswordHash, 1, User::Status::ACTIVE);

    EXPECT_CALL(*userRepo, read(1))
        .WillOnce(::testing::Return(user));

    EXPECT_CALL(*userRepo, update(::testing::_))
        .WillOnce(::testing::Return(user));

    // Should not throw
    EXPECT_NO_THROW(
        authService->changePassword(1, "OldPass123!", "NewPass456!")
    );
}

/**
 * @brief Test password change with wrong old password
 */
TEST_F(AuthenticationServiceTest, ChangePasswordWrongOldPassword) {
    std::string passwordHash = sms::utilities::PasswordHasher::hash("OldPass123!");
    User user(1, "student@test.edu", passwordHash, 1, User::Status::ACTIVE);

    EXPECT_CALL(*userRepo, read(1))
        .WillOnce(::testing::Return(user));

    EXPECT_THROW(
        authService->changePassword(1, "WrongOldPass123!", "NewPass456!"),
        std::runtime_error
    );
}
