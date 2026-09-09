#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/app/enrollments/services/EnrollmentService.hpp"

using namespace sms::app::enrollments;
using namespace sms::app::enrollments::domain;
using namespace sms::app::enrollments::services;
using namespace sms::app::enrollments::repositories;

/**
 * @brief Mock Enrollment Repository
 */
class MockEnrollmentRepository : public IEnrollmentRepository {
public:
    MOCK_METHOD(Enrollment, create, (const Enrollment&), (override));
    MOCK_METHOD(std::optional<Enrollment>, read, (int64_t), (const, override));
    MOCK_METHOD(Enrollment, update, (const Enrollment&), (override));
    MOCK_METHOD(bool, delete_by_id, (int64_t), (override));
    MOCK_METHOD(std::vector<Enrollment>, readAll, (), (const, override));
    MOCK_METHOD(std::vector<Enrollment>, readPage, (int64_t, int64_t), (const, override));
    MOCK_METHOD(int64_t, count, (), (const, override));
    MOCK_METHOD(bool, exists, (int64_t), (const, override));
    MOCK_METHOD(std::vector<Enrollment>, findByStudent, (int64_t), (const, override));
    MOCK_METHOD(std::vector<Enrollment>, findByCourse, (int64_t), (const, override));
    MOCK_METHOD(std::optional<Enrollment>, findByStudentAndCourse, (int64_t, int64_t), (const, override));
    MOCK_METHOD(std::vector<Enrollment>, findBySemester, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Enrollment>, findByStatus, (EnrollmentStatus), (const, override));
    MOCK_METHOD(bool, isEnrolled, (int64_t, int64_t), (const, override));
    MOCK_METHOD(int64_t, countActiveEnrollments, (int64_t), (const, override));
    MOCK_METHOD(double, calculateStudentGPA, (int64_t), (const, override));
    MOCK_METHOD(void, updateStatus, (int64_t, EnrollmentStatus), (override));
    MOCK_METHOD(void, postGrade, (int64_t, double, const std::string&), (override));
    MOCK_METHOD(std::vector<Enrollment>, getUngradedEnrollments, (), (const, override));
};

class EnrollmentServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockEnrollmentRepository> enrollmentRepo;
    std::shared_ptr<EnrollmentService> enrollmentService;

    void SetUp() override {
        enrollmentRepo = std::make_shared<MockEnrollmentRepository>();
        enrollmentService = std::make_shared<EnrollmentService>(enrollmentRepo);
    }

    Enrollment createTestEnrollment(int64_t id = 1) {
        Enrollment enrollment(id, 1, 1);
        enrollment.setStatus(EnrollmentStatus::PENDING);
        enrollment.setSemester("FALL2026");
        enrollment.setYear(2026);
        enrollment.setCreditHours(3);
        return enrollment;
    }
};

/**
 * @brief Test successful enrollment
 */
TEST_F(EnrollmentServiceTest, EnrollStudentSuccess) {
    EXPECT_CALL(*enrollmentRepo, isEnrolled(1, 1))
        .WillOnce(::testing::Return(false));

    Enrollment enrolledStudent = createTestEnrollment();
    EXPECT_CALL(*enrollmentRepo, create(::testing::_))
        .WillOnce(::testing::Return(enrolledStudent));

    Enrollment result = enrollmentService->enrollStudent(1, 1, "FALL2026", 2026);

    EXPECT_EQ(result.studentId(), 1);
    EXPECT_EQ(result.courseId(), 1);
}

/**
 * @brief Test enrollment with already enrolled student
 */
TEST_F(EnrollmentServiceTest, EnrollStudentAlreadyEnrolled) {
    EXPECT_CALL(*enrollmentRepo, isEnrolled(1, 1))
        .WillOnce(::testing::Return(true));

    EXPECT_THROW(
        enrollmentService->enrollStudent(1, 1, "FALL2026", 2026),
        std::runtime_error
    );
}

/**
 * @brief Test get student enrollments
 */
TEST_F(EnrollmentServiceTest, GetStudentEnrollments) {
    std::vector<Enrollment> enrollments;
    enrollments.push_back(createTestEnrollment());

    EXPECT_CALL(*enrollmentRepo, findByStudent(1))
        .WillOnce(::testing::Return(enrollments));

    auto result = enrollmentService->getStudentEnrollments(1);
    EXPECT_EQ(result.size(), 1);
}

/**
 * @brief Test get course enrollments
 */
TEST_F(EnrollmentServiceTest, GetCourseEnrollments) {
    std::vector<Enrollment> enrollments;
    enrollments.push_back(createTestEnrollment());

    EXPECT_CALL(*enrollmentRepo, findByCourse(1))
        .WillOnce(::testing::Return(enrollments));

    auto result = enrollmentService->getCourseEnrollments(1);
    EXPECT_EQ(result.size(), 1);
}

/**
 * @brief Test drop course
 */
TEST_F(EnrollmentServiceTest, DropCourseSuccess) {
    Enrollment enrollment = createTestEnrollment();
    enrollment.setStatus(EnrollmentStatus::PENDING);

    EXPECT_CALL(*enrollmentRepo, read(1))
        .WillOnce(::testing::Return(enrollment));

    EXPECT_CALL(*enrollmentRepo, updateStatus(1, EnrollmentStatus::DROPPED))
        .Times(1);

    EXPECT_NO_THROW(enrollmentService->dropCourse(1));
}

/**
 * @brief Test withdraw from course
 */
TEST_F(EnrollmentServiceTest, WithdrawFromCourseSuccess) {
    Enrollment enrollment = createTestEnrollment();
    enrollment.setStatus(EnrollmentStatus::ACTIVE);

    EXPECT_CALL(*enrollmentRepo, read(1))
        .WillOnce(::testing::Return(enrollment));

    EXPECT_CALL(*enrollmentRepo, updateStatus(1, EnrollmentStatus::WITHDRAWN))
        .Times(1);

    EXPECT_NO_THROW(enrollmentService->withdrawFromCourse(1));
}

/**
 * @brief Test approve enrollment
 */
TEST_F(EnrollmentServiceTest, ApproveEnrollmentSuccess) {
    Enrollment enrollment = createTestEnrollment();
    enrollment.setStatus(EnrollmentStatus::PENDING);

    EXPECT_CALL(*enrollmentRepo, read(1))
        .WillOnce(::testing::Return(enrollment));

    EXPECT_CALL(*enrollmentRepo, updateStatus(1, EnrollmentStatus::APPROVED))
        .Times(1);

    EXPECT_NO_THROW(enrollmentService->approveEnrollment(1));
}

/**
 * @brief Test activate enrollment
 */
TEST_F(EnrollmentServiceTest, ActivateEnrollmentSuccess) {
    Enrollment enrollment = createTestEnrollment();
    enrollment.setStatus(EnrollmentStatus::APPROVED);

    EXPECT_CALL(*enrollmentRepo, read(1))
        .WillOnce(::testing::Return(enrollment));

    EXPECT_CALL(*enrollmentRepo, updateStatus(1, EnrollmentStatus::ACTIVE))
        .Times(1);

    EXPECT_NO_THROW(enrollmentService->activateEnrollment(1));
}

/**
 * @brief Test complete enrollment
 */
TEST_F(EnrollmentServiceTest, CompleteEnrollmentSuccess) {
    Enrollment enrollment = createTestEnrollment();
    enrollment.setStatus(EnrollmentStatus::ACTIVE);

    EXPECT_CALL(*enrollmentRepo, read(1))
        .WillOnce(::testing::Return(enrollment));

    EXPECT_CALL(*enrollmentRepo, updateStatus(1, EnrollmentStatus::COMPLETED))
        .Times(1);

    EXPECT_NO_THROW(enrollmentService->completeEnrollment(1));
}

/**
 * @brief Test post grade
 */
TEST_F(EnrollmentServiceTest, PostGradeSuccess) {
    Enrollment enrollment = createTestEnrollment();
    enrollment.setStatus(EnrollmentStatus::COMPLETED);

    EXPECT_CALL(*enrollmentRepo, read(1))
        .WillOnce(::testing::Return(enrollment));

    EXPECT_CALL(*enrollmentRepo, postGrade(1, 85.0, "B"))
        .Times(1);

    EXPECT_NO_THROW(enrollmentService->postGrade(1, 85.0));
}

/**
 * @brief Test post grade with invalid grade
 */
TEST_F(EnrollmentServiceTest, PostGradeInvalidGrade) {
    EXPECT_THROW(
        enrollmentService->postGrade(1, 150.0),
        std::runtime_error
    );

    EXPECT_THROW(
        enrollmentService->postGrade(1, -10.0),
        std::runtime_error
    );
}

/**
 * @brief Test get student GPA
 */
TEST_F(EnrollmentServiceTest, GetStudentGPA) {
    EXPECT_CALL(*enrollmentRepo, calculateStudentGPA(1))
        .WillOnce(::testing::Return(3.5));

    double gpa = enrollmentService->getStudentGPA(1);
    EXPECT_DOUBLE_EQ(gpa, 3.5);
}

/**
 * @brief Test get pending grades
 */
TEST_F(EnrollmentServiceTest, GetPendingGrades) {
    std::vector<Enrollment> ungraded;
    ungraded.push_back(createTestEnrollment());

    EXPECT_CALL(*enrollmentRepo, getUngradedEnrollments())
        .WillOnce(::testing::Return(ungraded));

    auto result = enrollmentService->getPendingGrades();
    EXPECT_EQ(result.size(), 1);
}

/**
 * @brief Test generate transcript
 */
TEST_F(EnrollmentServiceTest, GenerateTranscript) {
    std::vector<Enrollment> enrollments;
    Enrollment enrollment = createTestEnrollment();
    enrollment.setStatus(EnrollmentStatus::COMPLETED);
    enrollment.setGradePosted(true);
    enrollment.setLetterGrade("A");
    enrollment.setGrade(95.0);
    enrollments.push_back(enrollment);

    EXPECT_CALL(*enrollmentRepo, findByStudent(1))
        .WillOnce(::testing::Return(enrollments));

    EXPECT_CALL(*enrollmentRepo, calculateStudentGPA(1))
        .WillOnce(::testing::Return(4.0));

    std::string transcript = enrollmentService->generateTranscript(1);
    EXPECT_FALSE(transcript.empty());
}

/**
 * @brief Test get enrollment statistics
 */
TEST_F(EnrollmentServiceTest, GetEnrollmentStatistics) {
    std::vector<Enrollment> enrollments;
    Enrollment enrollment1 = createTestEnrollment();
    enrollment1.setStatus(EnrollmentStatus::ACTIVE);
    
    Enrollment enrollment2 = createTestEnrollment(2);
    enrollment2.setStatus(EnrollmentStatus::COMPLETED);
    enrollment2.setGradePosted(true);
    enrollment2.setGrade(90.0);

    enrollments.push_back(enrollment1);
    enrollments.push_back(enrollment2);

    EXPECT_CALL(*enrollmentRepo, findByCourse(1))
        .WillOnce(::testing::Return(enrollments));

    std::string stats = enrollmentService->getEnrollmentStatistics(1);
    EXPECT_FALSE(stats.empty());
}

/**
 * @brief Test check prerequisites
 */
TEST_F(EnrollmentServiceTest, CheckPrerequisites) {
    bool result = enrollmentService->checkPrerequisites(1, 1);
    EXPECT_TRUE(result);
}
