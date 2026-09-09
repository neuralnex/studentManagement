#include <gtest/gtest.h>
#include "../../src/app/authentication/repositories/PostgreSQLUserRepository.hpp"
#include "../../src/app/authentication/repositories/PostgreSQLRoleRepository.hpp"
#include "../../src/app/authentication/repositories/PostgreSQLPermissionRepository.hpp"
#include "../../src/config/DatabaseConfig.hpp"
#include "../../src/database/ConnectionPool.hpp"

using namespace sms::app::authentication::domain;
using namespace sms::app::authentication::repositories;
using namespace sms::config;
using namespace sms::database;

/**
 * @brief Integration tests for authentication repositories
 * 
 * These tests interact with the actual database.
 * Requirements:
 * - Running PostgreSQL server
 * - Database initialized with schema migrations
 * - Environment variables: DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASSWORD
 * 
 * Note: Tests should be run in transaction blocks that rollback.
 */
class AuthenticationRepositoryTest : public ::testing::Test {
protected:
    std::shared_ptr<ConnectionPool> pool;
    std::shared_ptr<PostgreSQLRoleRepository> roleRepo;
    std::shared_ptr<PostgreSQLPermissionRepository> permissionRepo;
    std::shared_ptr<PostgreSQLUserRepository> userRepo;

    static bool IsDatabaseAvailable() {
        try {
            auto config = DatabaseConfig::loadFromEnvironment();
            if (!config) return false;
            
            ConnectionPool testPool(std::move(config));
            return testPool.isHealthy();
        } catch (...) {
            return false;
        }
    }

    void SetUp() override {
        if (!IsDatabaseAvailable()) {
            GTEST_SKIP() << "Database not available";
        }

        auto config = DatabaseConfig::loadFromEnvironment();
        pool = std::make_shared<ConnectionPool>(std::move(config));

        roleRepo = std::make_shared<PostgreSQLRoleRepository>(pool);
        permissionRepo = std::make_shared<PostgreSQLPermissionRepository>(pool);
        userRepo = std::make_shared<PostgreSQLUserRepository>(pool);
    }

    void TearDown() override {
        // In production, clean up test data here
        // For now, rely on test isolation strategy
    }

    /**
     * @brief Create a unique permission name for testing
     */
    std::string getUniquePermissionName() {
        static int counter = 0;
        return "TestPerm_" + std::to_string(++counter) + "_" +
               std::to_string(std::time(nullptr));
    }

    /**
     * @brief Create a unique role name for testing
     */
    std::string getUniqueRoleName() {
        static int counter = 0;
        return "TestRole_" + std::to_string(++counter) + "_" + 
               std::to_string(std::time(nullptr));
    }

    /**
     * @brief Create a unique user email for testing
     */
    std::string getUniqueEmail() {
        static int counter = 0;
        return "testuser" + std::to_string(++counter) + "_" + 
               std::to_string(std::time(nullptr)) + "@test.local";
    }
};

/**
 * @brief Test permission creation and retrieval
 */
TEST_F(AuthenticationRepositoryTest, CreateAndReadPermission) {
    std::string uniqueName = getUniquePermissionName();
    Permission perm(0, uniqueName, "Test permission description");

    Permission createdPerm = permissionRepo->create(perm);

    EXPECT_GT(createdPerm.id(), 0);
    EXPECT_EQ(createdPerm.name(), uniqueName);

    auto retrieved = permissionRepo->read(createdPerm.id());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name(), uniqueName);
}

/**
 * @brief Test role creation and retrieval
 */
TEST_F(AuthenticationRepositoryTest, CreateAndReadRole) {
    Role role(0, getUniqueRoleName(), "Test role description");
    
    Role createdRole = roleRepo->create(role);
    
    EXPECT_GT(createdRole.id(), 0);
    EXPECT_EQ(createdRole.status(), Role::Status::ACTIVE);
    
    auto retrieved = roleRepo->read(createdRole.id());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name(), role.name());
}

/**
 * @brief Test role with permissions
 */
TEST_F(AuthenticationRepositoryTest, GetRoleWithPermissions) {
    // Create role
    Role role(0, getUniqueRoleName(), "Role with permissions");
    Role createdRole = roleRepo->create(role);
    
    // Create and assign permissions
    Permission perm1(0, "perm_" + std::to_string(std::time(nullptr)) + "_1", "Permission 1");
    Permission perm2(0, "perm_" + std::to_string(std::time(nullptr)) + "_2", "Permission 2");
    
    Permission createdPerm1 = permissionRepo->create(perm1);
    Permission createdPerm2 = permissionRepo->create(perm2);
    
    roleRepo->assignPermission(createdRole.id(), createdPerm1.id());
    roleRepo->assignPermission(createdRole.id(), createdPerm2.id());
    
    // Retrieve role with permissions
    auto roleWithPerms = roleRepo->getRoleWithPermissions(createdRole.id());
    ASSERT_TRUE(roleWithPerms.has_value());
    EXPECT_EQ(roleWithPerms->permissions().size(), 2);
}

/**
 * @brief Test user creation and retrieval
 */
TEST_F(AuthenticationRepositoryTest, CreateAndReadUser) {
    // First create a role
    Role role(0, getUniqueRoleName(), "User test role");
    Role createdRole = roleRepo->create(role);
    
    // Create user
    std::string email = getUniqueEmail();
    User user(0, email, "hashed_password_hash", createdRole.id(), User::Status::ACTIVE);
    user.setFirstName("John");
    user.setLastName("Doe");
    
    User createdUser = userRepo->create(user);
    
    EXPECT_GT(createdUser.id(), 0);
    EXPECT_EQ(createdUser.email(), email);
    
    auto retrieved = userRepo->read(createdUser.id());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->email(), email);
    EXPECT_EQ(retrieved->firstName(), "John");
    EXPECT_EQ(retrieved->lastName(), "Doe");
}

/**
 * @brief Test finding user by email
 */
TEST_F(AuthenticationRepositoryTest, FindUserByEmail) {
    // Create role and user
    Role role(0, getUniqueRoleName(), "Test role");
    Role createdRole = roleRepo->create(role);
    
    std::string email = getUniqueEmail();
    User user(0, email, "password_hash", createdRole.id());
    User createdUser = userRepo->create(user);
    
    // Find by email
    auto found = userRepo->findByEmail(email);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id(), createdUser.id());
    EXPECT_EQ(found->email(), email);
}

/**
 * @brief Test email existence check
 */
TEST_F(AuthenticationRepositoryTest, EmailExists) {
    Role role(0, getUniqueRoleName(), "Test role");
    Role createdRole = roleRepo->create(role);
    
    std::string email = getUniqueEmail();
    User user(0, email, "password_hash", createdRole.id());
    userRepo->create(user);
    
    EXPECT_TRUE(userRepo->emailExists(email));
    EXPECT_FALSE(userRepo->emailExists("nonexistent@test.local"));
}

/**
 * @brief Test user update
 */
TEST_F(AuthenticationRepositoryTest, UpdateUser) {
    Role role(0, getUniqueRoleName(), "Test role");
    Role createdRole = roleRepo->create(role);
    
    std::string email = getUniqueEmail();
    User user(0, email, "password_hash", createdRole.id());
    User createdUser = userRepo->create(user);
    
    // Update user
    createdUser.setFirstName("Jane");
    createdUser.setLastName("Smith");
    User updated = userRepo->update(createdUser);
    
    EXPECT_EQ(updated.firstName(), "Jane");
    EXPECT_EQ(updated.lastName(), "Smith");
    
    // Verify in database
    auto retrieved = userRepo->read(createdUser.id());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->firstName(), "Jane");
}

/**
 * @brief Test user status changes
 */
TEST_F(AuthenticationRepositoryTest, UserStatusManagement) {
    Role role(0, getUniqueRoleName(), "Test role");
    Role createdRole = roleRepo->create(role);
    
    User user(0, getUniqueEmail(), "password_hash", createdRole.id());
    User createdUser = userRepo->create(user);
    
    // Lock account
    createdUser.setStatus(User::Status::LOCKED);
    userRepo->update(createdUser);
    
    auto updated = userRepo->read(createdUser.id());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status(), User::Status::LOCKED);
    
    // Find locked accounts
    auto locked = userRepo->findLockedAccounts(1);
    bool found = false;
    for (const auto& u : locked) {
        if (u.id() == createdUser.id()) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

/**
 * @brief Test finding users by role
 */
TEST_F(AuthenticationRepositoryTest, FindUsersByRole) {
    // Create role
    Role role(0, getUniqueRoleName(), "Test role");
    Role createdRole = roleRepo->create(role);
    
    // Create multiple users with same role
    User user1(0, getUniqueEmail(), "pass_hash_1", createdRole.id());
    User user2(0, getUniqueEmail(), "pass_hash_2", createdRole.id());
    
    User created1 = userRepo->create(user1);
    User created2 = userRepo->create(user2);
    
    // Find users by role
    auto usersInRole = userRepo->findByRole(createdRole.id());
    
    EXPECT_GE(usersInRole.size(), 2);
    
    bool found1 = false, found2 = false;
    for (const auto& u : usersInRole) {
        if (u.id() == created1.id()) found1 = true;
        if (u.id() == created2.id()) found2 = true;
    }
    
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

/**
 * @brief Test user count
 */
TEST_F(AuthenticationRepositoryTest, CountUsers) {
    int64_t initialCount = userRepo->count();
    
    // Create a role
    Role role(0, getUniqueRoleName(), "Test role");
    Role createdRole = roleRepo->create(role);
    
    // Create a user
    User user(0, getUniqueEmail(), "password_hash", createdRole.id());
    userRepo->create(user);
    
    int64_t newCount = userRepo->count();
    EXPECT_EQ(newCount, initialCount + 1);
}

/**
 * @brief Test pagination
 */
TEST_F(AuthenticationRepositoryTest, UserPagination) {
    Role role(0, getUniqueRoleName(), "Test role");
    Role createdRole = roleRepo->create(role);
    
    // Create multiple users
    for (int i = 0; i < 5; ++i) {
        User user(0, getUniqueEmail(), "password_hash", createdRole.id());
        userRepo->create(user);
    }
    
    // Get first page
    auto page1 = userRepo->readPage(0, 3);
    EXPECT_LE(page1.size(), 3);
    
    // Get second page
    auto page2 = userRepo->readPage(3, 3);
    EXPECT_LE(page2.size(), 3);
}
