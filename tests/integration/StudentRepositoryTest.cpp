#include <gtest/gtest.h>
#include "../../src/app/students/repositories/PostgreSQLStudentRepository.hpp"
#include "../../src/config/DatabaseConfig.hpp"
#include "../../src/database/ConnectionPool.hpp"

using namespace sms::app::students::domain;
using namespace sms::app::students::repositories;
using namespace sms::config;
using namespace sms::database;

/**
 * @brief Integration tests for student repository
 * 
 * Requires running PostgreSQL database with migrations applied.
 */
class StudentRepositoryTest : public ::testing::Test {
protected:
    std::shared_ptr<ConnectionPool> pool;
    std::shared_ptr<PostgreSQLStudentRepository> studentRepo;

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
        studentRepo = std::make_shared<PostgreSQLStudentRepository>(pool);
    }

    Student createTestStudent(int64_t userId, const std::string& matric) {
        Student student(0, userId, matric);
        student.setFirstName("Test");
        student.setLastName("Student");
        student.setEmail("test.student@university.edu");
        student.setPhoneNumber("1234567890");
        student.setStatus(StudentStatus::ACTIVE);
        return student;
    }
};

/**
 * @brief Test student creation
 */
TEST_F(StudentRepositoryTest, CreateStudentSuccess) {
    Student student = createTestStudent(1, "TEST-STU-001");
    
    Student created = studentRepo->create(student);
    
    EXPECT_GT(created.id(), 0);
    EXPECT_EQ(created.matricNumber(), "TEST-STU-001");
    EXPECT_EQ(created.status(), StudentStatus::ACTIVE);
}

/**
 * @brief Test student retrieval
 */
TEST_F(StudentRepositoryTest, ReadStudentSuccess) {
    Student student = createTestStudent(1, "TEST-STU-002");
    Student created = studentRepo->create(student);
    
    auto retrieved = studentRepo->read(created.id());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->matricNumber(), "TEST-STU-002");
    EXPECT_EQ(retrieved->firstName(), "Test");
}

/**
 * @brief Test finding student by matric number
 */
TEST_F(StudentRepositoryTest, FindByMatricNumberSuccess) {
    Student student = createTestStudent(1, "TEST-STU-003");
    Student created = studentRepo->create(student);
    
    auto found = studentRepo->findByMatricNumber("TEST-STU-003");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id(), created.id());
}

/**
 * @brief Test matric number existence check
 */
TEST_F(StudentRepositoryTest, MatricNumberExists) {
    Student student = createTestStudent(1, "TEST-STU-004");
    studentRepo->create(student);
    
    EXPECT_TRUE(studentRepo->matricNumberExists("TEST-STU-004"));
    EXPECT_FALSE(studentRepo->matricNumberExists("TEST-STU-999"));
}

/**
 * @brief Test finding student by user ID
 */
TEST_F(StudentRepositoryTest, FindByUserIdSuccess) {
    Student student = createTestStudent(99, "TEST-STU-005");
    Student created = studentRepo->create(student);
    
    auto found = studentRepo->findByUserId(99);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->userId(), 99);
}

/**
 * @brief Test updating student
 */
TEST_F(StudentRepositoryTest, UpdateStudentSuccess) {
    Student student = createTestStudent(1, "TEST-STU-006");
    Student created = studentRepo->create(student);
    
    created.setFirstName("Updated");
    Student updated = studentRepo->update(created);
    
    EXPECT_EQ(updated.firstName(), "Updated");
    
    auto retrieved = studentRepo->read(created.id());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->firstName(), "Updated");
}

/**
 * @brief Test finding students by status
 */
TEST_F(StudentRepositoryTest, FindByStatusSuccess) {
    Student active = createTestStudent(1, "TEST-STU-007");
    active.setStatus(StudentStatus::ACTIVE);
    studentRepo->create(active);
    
    auto activeStudents = studentRepo->findByStatus(StudentStatus::ACTIVE);
    EXPECT_GT(activeStudents.size(), 0);
}

/**
 * @brief Test updating academic standing
 */
TEST_F(StudentRepositoryTest, UpdateAcademicStandingSuccess) {
    Student student = createTestStudent(1, "TEST-STU-008");
    Student created = studentRepo->create(student);
    
    studentRepo->updateAcademicStanding(created.id(), AcademicStanding::PROBATION);
    
    auto updated = studentRepo->read(created.id());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->academicStanding(), AcademicStanding::PROBATION);
}

/**
 * @brief Test searching by name
 */
TEST_F(StudentRepositoryTest, SearchByNameSuccess) {
    Student student = createTestStudent(1, "TEST-STU-009");
    student.setFirstName("John");
    studentRepo->create(student);
    
    auto results = studentRepo->searchByName("John");
    EXPECT_GT(results.size(), 0);
    
    bool found = false;
    for (const auto& s : results) {
        if (s.matricNumber() == "TEST-STU-009") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

/**
 * @brief Test count students
 */
TEST_F(StudentRepositoryTest, CountStudents) {
    int64_t initialCount = studentRepo->count();
    
    Student student = createTestStudent(1, "TEST-STU-010");
    studentRepo->create(student);
    
    int64_t newCount = studentRepo->count();
    EXPECT_EQ(newCount, initialCount + 1);
}

/**
 * @brief Test pagination
 */
TEST_F(StudentRepositoryTest, PaginationSuccess) {
    // Create multiple students
    for (int i = 0; i < 5; ++i) {
        Student student = createTestStudent(1, "TEST-STU-PAGE-" + std::to_string(i));
        studentRepo->create(student);
    }
    
    auto page1 = studentRepo->readPage(0, 3);
    EXPECT_LE(page1.size(), 3);
    
    auto page2 = studentRepo->readPage(3, 3);
    EXPECT_LE(page2.size(), 3);
}

/**
 * @brief Test counting active students
 */
TEST_F(StudentRepositoryTest, CountActiveStudents) {
    Student student = createTestStudent(1, "TEST-STU-ACTIVE");
    student.setStatus(StudentStatus::ACTIVE);
    studentRepo->create(student);
    
    int64_t activeCount = studentRepo->countActiveStudents();
    EXPECT_GT(activeCount, 0);
}

/**
 * @brief Test soft delete
 */
TEST_F(StudentRepositoryTest, SoftDeleteSuccess) {
    Student student = createTestStudent(1, "TEST-STU-DELETE");
    Student created = studentRepo->create(student);
    
    bool deleted = studentRepo->delete_by_id(created.id());
    EXPECT_TRUE(deleted);
    
    // Student should not be retrievable after soft delete
    auto retrieved = studentRepo->read(created.id());
    EXPECT_FALSE(retrieved.has_value());
}
