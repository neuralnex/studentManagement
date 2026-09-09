#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/app/students/services/StudentService.hpp"

using namespace sms::app::students;
using namespace sms::app::students::domain;
using namespace sms::app::students::services;
using namespace sms::app::students::repositories;

/**
 * @brief Mock Student Repository
 */
class MockStudentRepository : public IStudentRepository {
public:
    MOCK_METHOD(Student, create, (const Student&), (override));
    MOCK_METHOD(std::optional<Student>, read, (int64_t), (const, override));
    MOCK_METHOD(Student, update, (const Student&), (override));
    MOCK_METHOD(bool, delete_by_id, (int64_t), (override));
    MOCK_METHOD(std::vector<Student>, readAll, (), (const, override));
    MOCK_METHOD(std::vector<Student>, readPage, (int64_t, int64_t), (const, override));
    MOCK_METHOD(int64_t, count, (), (const, override));
    MOCK_METHOD(bool, exists, (int64_t), (const, override));
    MOCK_METHOD(std::optional<Student>, findByMatricNumber, (const std::string&), (const, override));
    MOCK_METHOD(std::optional<Student>, findByUserId, (int64_t), (const, override));
    MOCK_METHOD(std::vector<Student>, findByStatus, (StudentStatus), (const, override));
    MOCK_METHOD(std::vector<Student>, findByAcademicStanding, (AcademicStanding), (const, override));
    MOCK_METHOD(std::vector<Student>, searchByName, (const std::string&), (const, override));
    MOCK_METHOD(bool, matricNumberExists, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Student>, getStudentsOnProbation, (), (const, override));
    MOCK_METHOD(int64_t, countActiveStudents, (), (const, override));
    MOCK_METHOD(std::vector<Student>, getGraduatedStudents, (int64_t, int64_t), (const, override));
    MOCK_METHOD(void, updateAcademicStanding, (int64_t, AcademicStanding), (override));
    MOCK_METHOD(std::vector<Student>, findByNameParts, (const std::string&, const std::string&), (const, override));
};

class StudentServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockStudentRepository> studentRepo;
    std::shared_ptr<StudentService> studentService;

    void SetUp() override {
        studentRepo = std::make_shared<MockStudentRepository>();
        studentService = std::make_shared<StudentService>(studentRepo);
    }

    Student createTestStudent(int64_t id = 1) {
        Student student(id, 1, "STU-2026-001");
        student.setFirstName("John");
        student.setLastName("Doe");
        student.setEmail("john.doe@university.edu");
        student.setPhoneNumber("1234567890");
        student.setStatus(StudentStatus::ACTIVE);
        return student;
    }
};

/**
 * @brief Test successful student registration
 */
TEST_F(StudentServiceTest, RegisterStudentSuccess) {
    EXPECT_CALL(*studentRepo, matricNumberExists("STU-2026-001"))
        .WillOnce(::testing::Return(false));

    Student createdStudent = createTestStudent();
    EXPECT_CALL(*studentRepo, create(::testing::_))
        .WillOnce(::testing::Return(createdStudent));

    Student result = studentService->registerStudent(
        1,
        "STU-2026-001",
        "John",
        "Doe",
        "john.doe@university.edu",
        "1234567890"
    );

    EXPECT_EQ(result.matricNumber(), "STU-2026-001");
    EXPECT_EQ(result.firstName(), "John");
    EXPECT_EQ(result.lastName(), "Doe");
}

/**
 * @brief Test registration with existing matric number
 */
TEST_F(StudentServiceTest, RegisterStudentMatricExists) {
    EXPECT_CALL(*studentRepo, matricNumberExists("STU-2026-001"))
        .WillOnce(::testing::Return(true));

    EXPECT_THROW(
        studentService->registerStudent(
            1,
            "STU-2026-001",
            "John",
            "Doe",
            "john.doe@university.edu",
            "1234567890"
        ),
        std::runtime_error
    );
}

/**
 * @brief Test registration with invalid email
 */
TEST_F(StudentServiceTest, RegisterStudentInvalidEmail) {
    EXPECT_THROW(
        studentService->registerStudent(
            1,
            "STU-2026-001",
            "John",
            "Doe",
            "invalid-email",
            "1234567890"
        ),
        std::runtime_error
    );
}

/**
 * @brief Test get student by ID
 */
TEST_F(StudentServiceTest, GetStudentSuccess) {
    Student student = createTestStudent();
    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    auto result = studentService->getStudent(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->email(), "john.doe@university.edu");
}

/**
 * @brief Test get non-existent student
 */
TEST_F(StudentServiceTest, GetStudentNotFound) {
    EXPECT_CALL(*studentRepo, read(999))
        .WillOnce(::testing::Return(std::nullopt));

    auto result = studentService->getStudent(999);
    EXPECT_FALSE(result.has_value());
}

/**
 * @brief Test get student by matric number
 */
TEST_F(StudentServiceTest, GetStudentByMatricNumber) {
    Student student = createTestStudent();
    EXPECT_CALL(*studentRepo, findByMatricNumber("STU-2026-001"))
        .WillOnce(::testing::Return(student));

    auto result = studentService->getStudentByMatricNumber("STU-2026-001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->matricNumber(), "STU-2026-001");
}

/**
 * @brief Test get student by user ID
 */
TEST_F(StudentServiceTest, GetStudentByUserId) {
    Student student = createTestStudent();
    EXPECT_CALL(*studentRepo, findByUserId(1))
        .WillOnce(::testing::Return(student));

    auto result = studentService->getStudentByUserId(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->userId(), 1);
}

/**
 * @brief Test update student
 */
TEST_F(StudentServiceTest, UpdateStudentSuccess) {
    Student student = createTestStudent();
    Student updated = createTestStudent();
    updated.setFirstName("Jane");

    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    EXPECT_CALL(*studentRepo, update(::testing::_))
        .WillOnce(::testing::Return(updated));

    Student result = studentService->updateStudent(updated);
    EXPECT_EQ(result.firstName(), "Jane");
}

/**
 * @brief Test suspend student
 */
TEST_F(StudentServiceTest, SuspendStudentSuccess) {
    Student student = createTestStudent();
    Student suspended = createTestStudent();
    suspended.setStatus(StudentStatus::SUSPENDED);

    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    EXPECT_CALL(*studentRepo, update(::testing::_))
        .WillOnce(::testing::Return(suspended));

    EXPECT_NO_THROW(studentService->suspendStudent(1, "Academic misconduct"));
}

/**
 * @brief Test reactivate student
 */
TEST_F(StudentServiceTest, ReactivateStudentSuccess) {
    Student student = createTestStudent();
    student.setStatus(StudentStatus::SUSPENDED);

    Student reactivated = createTestStudent();
    reactivated.setStatus(StudentStatus::ACTIVE);

    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    EXPECT_CALL(*studentRepo, update(::testing::_))
        .WillOnce(::testing::Return(reactivated));

    EXPECT_NO_THROW(studentService->reactivateStudent(1));
}

/**
 * @brief Test graduate student
 */
TEST_F(StudentServiceTest, GraduateStudentSuccess) {
    Student student = createTestStudent();
    Student graduated = createTestStudent();
    graduated.setStatus(StudentStatus::GRADUATED);

    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    EXPECT_CALL(*studentRepo, update(::testing::_))
        .WillOnce(::testing::Return(graduated));

    EXPECT_NO_THROW(studentService->graduateStudent(1));
}

/**
 * @brief Test delete student
 */
TEST_F(StudentServiceTest, DeleteStudentSuccess) {
    EXPECT_CALL(*studentRepo, delete_by_id(1))
        .WillOnce(::testing::Return(true));

    bool result = studentService->deleteStudent(1);
    EXPECT_TRUE(result);
}

/**
 * @brief Test search students
 */
TEST_F(StudentServiceTest, SearchStudentsSuccess) {
    std::vector<Student> students;
    students.push_back(createTestStudent());

    EXPECT_CALL(*studentRepo, searchByName("John"))
        .WillOnce(::testing::Return(students));

    auto result = studentService->searchStudents("John");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].firstName(), "John");
}

/**
 * @brief Test search students with empty term
 */
TEST_F(StudentServiceTest, SearchStudentsEmptyTerm) {
    EXPECT_THROW(
        studentService->searchStudents(""),
        std::runtime_error
    );
}

/**
 * @brief Test get active students
 */
TEST_F(StudentServiceTest, GetActiveStudents) {
    std::vector<Student> students;
    students.push_back(createTestStudent());

    EXPECT_CALL(*studentRepo, readPage(0, 10))
        .WillOnce(::testing::Return(students));

    auto result = studentService->getActiveStudents(0, 10);
    EXPECT_EQ(result.size(), 1);
}

/**
 * @brief Test update academic standing
 */
TEST_F(StudentServiceTest, UpdateAcademicStandingSuccess) {
    Student student = createTestStudent();

    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    EXPECT_CALL(*studentRepo, updateAcademicStanding(1, AcademicStanding::PROBATION))
        .Times(1);

    EXPECT_NO_THROW(
        studentService->updateAcademicStanding(1, AcademicStanding::PROBATION)
    );
}

/**
 * @brief Test set emergency contact
 */
TEST_F(StudentServiceTest, SetEmergencyContactSuccess) {
    Student student = createTestStudent();
    Student updated = createTestStudent();
    updated.setEmergencyContactName("Jane Doe");
    updated.setEmergencyContactPhone("9876543210");

    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    EXPECT_CALL(*studentRepo, update(::testing::_))
        .WillOnce(::testing::Return(updated));

    EXPECT_NO_THROW(
        studentService->setEmergencyContact(1, "Jane Doe", "9876543210")
    );
}

/**
 * @brief Test set guardian info
 */
TEST_F(StudentServiceTest, SetGuardianInfoSuccess) {
    Student student = createTestStudent();
    Student updated = createTestStudent();
    updated.setGuardianName("Parent Name");
    updated.setGuardianPhone("5555555555");
    updated.setGuardianEmail("parent@email.com");

    EXPECT_CALL(*studentRepo, read(1))
        .WillOnce(::testing::Return(student));

    EXPECT_CALL(*studentRepo, update(::testing::_))
        .WillOnce(::testing::Return(updated));

    EXPECT_NO_THROW(
        studentService->setGuardianInfo(1, "Parent Name", "5555555555", "parent@email.com")
    );
}

/**
 * @brief Test get student statistics
 */
TEST_F(StudentServiceTest, GetStudentStatistics) {
    EXPECT_CALL(*studentRepo, count())
        .WillOnce(::testing::Return(100));

    EXPECT_CALL(*studentRepo, countActiveStudents())
        .WillOnce(::testing::Return(85));

    EXPECT_CALL(*studentRepo, getStudentsOnProbation())
        .WillOnce(::testing::Return(std::vector<Student>()));

    EXPECT_CALL(*studentRepo, findByStatus(StudentStatus::SUSPENDED))
        .WillOnce(::testing::Return(std::vector<Student>()));

    EXPECT_CALL(*studentRepo, findByStatus(StudentStatus::GRADUATED))
        .WillOnce(::testing::Return(std::vector<Student>()));

    std::string stats = studentService->getStudentStatistics();
    EXPECT_FALSE(stats.empty());
}
