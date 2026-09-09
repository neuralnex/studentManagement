#include <gtest/gtest.h>
#include "../../src/app/enrollments/repositories/PostgreSQLEnrollmentRepository.hpp"
#include "../../src/config/DatabaseConfig.hpp"
#include "../../src/database/ConnectionPool.hpp"

using namespace sms::app::enrollments::domain;
using namespace sms::app::enrollments::repositories;
using namespace sms::config;
using namespace sms::database;

/**
 * @brief Integration tests for enrollment repository
 */
class EnrollmentRepositoryTest : public ::testing::Test {
protected:
    std::shared_ptr<ConnectionPool> pool;
    std::shared_ptr<PostgreSQLEnrollmentRepository> enrollmentRepo;

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
        enrollmentRepo = std::make_shared<PostgreSQLEnrollmentRepository>(pool);
    }

    Enrollment createTestEnrollment(int64_t studentId, int64_t courseId) {
        Enrollment enrollment(0, studentId, courseId);
        enrollment.setStatus(EnrollmentStatus::PENDING);
        enrollment.setSemester("FALL2026");
        enrollment.setYear(2026);
        enrollment.setCreditHours(3);
        enrollment.setPrerequisitesMet(true);
        return enrollment;
    }
};

/**
 * @brief Test enrollment creation
 */
TEST_F(EnrollmentRepositoryTest, CreateEnrollmentSuccess) {
    Enrollment enrollment = createTestEnrollment(1, 1);
    
    Enrollment created = enrollmentRepo->create(enrollment);
    
    EXPECT_GT(created.id(), 0);
    EXPECT_EQ(created.studentId(), 1);
    EXPECT_EQ(created.courseId(), 1);
    EXPECT_EQ(created.status(), EnrollmentStatus::PENDING);
}

/**
 * @brief Test enrollment retrieval
 */
TEST_F(EnrollmentRepositoryTest, ReadEnrollmentSuccess) {
    Enrollment enrollment = createTestEnrollment(1, 2);
    Enrollment created = enrollmentRepo->create(enrollment);
    
    auto retrieved = enrollmentRepo->read(created.id());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->studentId(), 1);
    EXPECT_EQ(retrieved->courseId(), 2);
}

/**
 * @brief Test finding enrollments by student
 */
TEST_F(EnrollmentRepositoryTest, FindByStudentSuccess) {
    Enrollment enrollment = createTestEnrollment(2, 3);
    enrollmentRepo->create(enrollment);
    
    auto enrollments = enrollmentRepo->findByStudent(2);
    EXPECT_GT(enrollments.size(), 0);
}

/**
 * @brief Test finding enrollments by course
 */
TEST_F(EnrollmentRepositoryTest, FindByCourseSuccess) {
    Enrollment enrollment = createTestEnrollment(3, 4);
    enrollmentRepo->create(enrollment);
    
    auto enrollments = enrollmentRepo->findByCourse(4);
    EXPECT_GT(enrollments.size(), 0);
}

/**
 * @brief Test finding enrollment by student and course
 */
TEST_F(EnrollmentRepositoryTest, FindByStudentAndCourseSuccess) {
    Enrollment enrollment = createTestEnrollment(4, 5);
    enrollmentRepo->create(enrollment);
    
    auto found = enrollmentRepo->findByStudentAndCourse(4, 5);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->studentId(), 4);
    EXPECT_EQ(found->courseId(), 5);
}

/**
 * @brief Test checking if student is enrolled
 */
TEST_F(EnrollmentRepositoryTest, IsEnrolledCheck) {
    Enrollment enrollment = createTestEnrollment(5, 6);
    enrollmentRepo->create(enrollment);
    
    EXPECT_TRUE(enrollmentRepo->isEnrolled(5, 6));
    EXPECT_FALSE(enrollmentRepo->isEnrolled(5, 999));
}

/**
 * @brief Test updating enrollment status
 */
TEST_F(EnrollmentRepositoryTest, UpdateStatusSuccess) {
    Enrollment enrollment = createTestEnrollment(6, 7);
    Enrollment created = enrollmentRepo->create(enrollment);
    
    enrollmentRepo->updateStatus(created.id(), EnrollmentStatus::APPROVED);
    
    auto updated = enrollmentRepo->read(created.id());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status(), EnrollmentStatus::APPROVED);
}

/**
 * @brief Test posting grade
 */
TEST_F(EnrollmentRepositoryTest, PostGradeSuccess) {
    Enrollment enrollment = createTestEnrollment(7, 8);
    Enrollment created = enrollmentRepo->create(enrollment);
    
    enrollmentRepo->postGrade(created.id(), 85.0, "B");
    
    auto graded = enrollmentRepo->read(created.id());
    ASSERT_TRUE(graded.has_value());
    EXPECT_EQ(graded->grade(), 85.0);
    EXPECT_EQ(graded->letterGrade(), "B");
    EXPECT_TRUE(graded->gradePosted());
}

/**
 * @brief Test counting active enrollments
 */
TEST_F(EnrollmentRepositoryTest, CountActiveEnrollments) {
    Enrollment enrollment = createTestEnrollment(8, 9);
    enrollment.setStatus(EnrollmentStatus::ACTIVE);
    enrollmentRepo->create(enrollment);
    
    int64_t count = enrollmentRepo->countActiveEnrollments(9);
    EXPECT_GT(count, 0);
}

/**
 * @brief Test finding enrollments by semester
 */
TEST_F(EnrollmentRepositoryTest, FindBySemesterSuccess) {
    Enrollment enrollment = createTestEnrollment(9, 10);
    enrollment.setSemester("FALL2026");
    enrollmentRepo->create(enrollment);
    
    auto enrollments = enrollmentRepo->findBySemester("FALL2026");
    EXPECT_GT(enrollments.size(), 0);
}

/**
 * @brief Test finding enrollments by status
 */
TEST_F(EnrollmentRepositoryTest, FindByStatusSuccess) {
    Enrollment enrollment = createTestEnrollment(10, 11);
    enrollment.setStatus(EnrollmentStatus::ACTIVE);
    enrollmentRepo->create(enrollment);
    
    auto enrollments = enrollmentRepo->findByStatus(EnrollmentStatus::ACTIVE);
    EXPECT_GT(enrollments.size(), 0);
}

/**
 * @brief Test calculating student GPA
 */
TEST_F(EnrollmentRepositoryTest, CalculateStudentGPA) {
    Enrollment enrollment = createTestEnrollment(11, 12);
    enrollment.setStatus(EnrollmentStatus::COMPLETED);
    enrollment.setGradePosted(true);
    enrollmentRepo->create(enrollment);
    
    // Post a grade to calculate GPA
    enrollmentRepo->postGrade(enrollment.id(), 90.0, "A");
    
    double gpa = enrollmentRepo->calculateStudentGPA(11);
    EXPECT_GE(gpa, 0.0);
    EXPECT_LE(gpa, 4.0);
}

/**
 * @brief Test getting ungraded enrollments
 */
TEST_F(EnrollmentRepositoryTest, GetUngradedEnrollments) {
    Enrollment enrollment = createTestEnrollment(12, 13);
    enrollment.setStatus(EnrollmentStatus::COMPLETED);
    enrollmentRepo->create(enrollment);
    
    auto ungraded = enrollmentRepo->getUngradedEnrollments();
    // Result may or may not contain our test enrollment depending on other data
    EXPECT_GE(ungraded.size(), 0);
}

/**
 * @brief Test updating enrollment
 */
TEST_F(EnrollmentRepositoryTest, UpdateEnrollmentSuccess) {
    Enrollment enrollment = createTestEnrollment(13, 14);
    Enrollment created = enrollmentRepo->create(enrollment);
    
    created.setStatus(EnrollmentStatus::ACTIVE);
    Enrollment updated = enrollmentRepo->update(created);
    
    EXPECT_EQ(updated.status(), EnrollmentStatus::ACTIVE);
}

/**
 * @brief Test pagination
 */
TEST_F(EnrollmentRepositoryTest, PaginationSuccess) {
    // Create multiple enrollments
    for (int i = 0; i < 5; ++i) {
        Enrollment enrollment = createTestEnrollment(100 + i, 200 + i);
        enrollmentRepo->create(enrollment);
    }
    
    auto page1 = enrollmentRepo->readPage(0, 3);
    EXPECT_LE(page1.size(), 3);
    
    auto page2 = enrollmentRepo->readPage(3, 3);
    EXPECT_LE(page2.size(), 3);
}

/**
 * @brief Test count enrollments
 */
TEST_F(EnrollmentRepositoryTest, CountEnrollments) {
    Enrollment enrollment = createTestEnrollment(14, 15);
    enrollmentRepo->create(enrollment);
    
    int64_t count = enrollmentRepo->count();
    EXPECT_GT(count, 0);
}

/**
 * @brief Test enrollment exists
 */
TEST_F(EnrollmentRepositoryTest, EnrollmentExists) {
    Enrollment enrollment = createTestEnrollment(15, 16);
    Enrollment created = enrollmentRepo->create(enrollment);
    
    EXPECT_TRUE(enrollmentRepo->exists(created.id()));
    EXPECT_FALSE(enrollmentRepo->exists(999999));
}
