#pragma once

#include "IStudentService.hpp"
#include <memory>

namespace sms::app::students::services {

/**
 * @brief Student service implementation
 */
class StudentService : public IStudentService {
public:
    /**
     * @brief Constructor
     * @param studentRepo Student repository
     */
    explicit StudentService(std::shared_ptr<IStudentRepository> studentRepo);

    Student registerStudent(
        int64_t userId,
        const std::string& matricNumber,
        const std::string& firstName,
        const std::string& lastName,
        const std::string& email,
        const std::string& phoneNumber
    ) override;

    std::optional<Student> getStudent(int64_t studentId) const override;

    std::optional<Student> getStudentByMatricNumber(const std::string& matricNumber) const override;

    std::optional<Student> getStudentByUserId(int64_t userId) const override;

    Student updateStudent(const Student& student) override;

    void suspendStudent(int64_t studentId, const std::string& reason = "") override;

    void reactivateStudent(int64_t studentId) override;

    void graduateStudent(int64_t studentId) override;

    bool deleteStudent(int64_t studentId) override;

    std::vector<Student> searchStudents(const std::string& searchTerm) const override;

    std::vector<Student> getActiveStudents(int64_t offset, int64_t limit) const override;

    std::vector<Student> getStudentsOnProbation() const override;

    void updateAcademicStanding(int64_t studentId, AcademicStanding standing) override;

    std::string getStudentStatistics() const override;

    int64_t getTotalStudentCount() const override;

    int64_t getActiveStudentCount() const override;

    void setEmergencyContact(
        int64_t studentId,
        const std::string& contactName,
        const std::string& contactPhone
    ) override;

    void setGuardianInfo(
        int64_t studentId,
        const std::string& guardianName,
        const std::string& guardianPhone,
        const std::string& guardianEmail
    ) override;

private:
    std::shared_ptr<IStudentRepository> m_studentRepo;

    /**
     * @brief Validate student input
     * @param firstName First name
     * @param lastName Last name
     * @param email Email
     * @throws std::runtime_error on validation failure
     */
    void validateStudentInput(
        const std::string& firstName,
        const std::string& lastName,
        const std::string& email
    ) const;

    /**
     * @brief Validate email format
     * @param email Email to validate
     * @throws std::runtime_error if invalid
     */
    void validateEmail(const std::string& email) const;
};

} // namespace sms::app::students::services
