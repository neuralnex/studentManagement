#pragma once

#include "../domain/Entities.hpp"
#include "../repositories/IStudentRepository.hpp"
#include <optional>
#include <vector>
#include <memory>

namespace sms::app::students::services {

using namespace sms::app::students::domain;
using namespace sms::app::students::repositories;

/**
 * @brief Student service interface
 * 
 * High-level business operations for student management
 */
class IStudentService {
public:
    virtual ~IStudentService() = default;

    /**
     * @brief Register a new student
     * @param userId Associated user ID
     * @param matricNumber Unique matric number
     * @param firstName Student's first name
     * @param lastName Student's last name
     * @param email Email address
     * @param phoneNumber Phone number
     * @return Created student
     * @throws std::runtime_error on validation failure
     */
    virtual Student registerStudent(
        int64_t userId,
        const std::string& matricNumber,
        const std::string& firstName,
        const std::string& lastName,
        const std::string& email,
        const std::string& phoneNumber
    ) = 0;

    /**
     * @brief Get student by ID
     * @param studentId Student ID
     * @return Student if found, std::nullopt otherwise
     */
    virtual std::optional<Student> getStudent(int64_t studentId) const = 0;

    /**
     * @brief Get student by matric number
     * @param matricNumber Matric number
     * @return Student if found, std::nullopt otherwise
     */
    virtual std::optional<Student> getStudentByMatricNumber(const std::string& matricNumber) const = 0;

    /**
     * @brief Get student by user ID
     * @param userId Associated user ID
     * @return Student if found, std::nullopt otherwise
     */
    virtual std::optional<Student> getStudentByUserId(int64_t userId) const = 0;

    /**
     * @brief Update student information
     * @param student Student with updated data
     * @return Updated student
     * @throws std::runtime_error on failure
     */
    virtual Student updateStudent(const Student& student) = 0;

    /**
     * @brief Suspend a student
     * @param studentId Student ID
     * @param reason Reason for suspension (optional)
     * @throws std::runtime_error on failure
     */
    virtual void suspendStudent(int64_t studentId, const std::string& reason = "") = 0;

    /**
     * @brief Reactivate a suspended student
     * @param studentId Student ID
     * @throws std::runtime_error on failure
     */
    virtual void reactivateStudent(int64_t studentId) = 0;

    /**
     * @brief Graduate a student
     * @param studentId Student ID
     * @throws std::runtime_error on failure
     */
    virtual void graduateStudent(int64_t studentId) = 0;

    /**
     * @brief Delete a student (soft delete)
     * @param studentId Student ID
     * @return True if deleted, false if not found
     */
    virtual bool deleteStudent(int64_t studentId) = 0;

    /**
     * @brief Search students by name
     * @param searchTerm First name, last name, or email
     * @return Vector of matching students
     */
    virtual std::vector<Student> searchStudents(const std::string& searchTerm) const = 0;

    /**
     * @brief Get all active students
     * @param offset Pagination offset
     * @param limit Pagination limit
     * @return Vector of active students
     */
    virtual std::vector<Student> getActiveStudents(int64_t offset, int64_t limit) const = 0;

    /**
     * @brief Get students on academic probation
     * @return Vector of students on probation
     */
    virtual std::vector<Student> getStudentsOnProbation() const = 0;

    /**
     * @brief Update student's academic standing
     * @param studentId Student ID
     * @param standing New standing
     * @throws std::runtime_error on failure
     */
    virtual void updateAcademicStanding(int64_t studentId, AcademicStanding standing) = 0;

    /**
     * @brief Get student statistics
     * @return JSON object with stats (total, active, graduated, etc.)
     */
    virtual std::string getStudentStatistics() const = 0;

    /**
     * @brief Get total number of students
     * @return Student count
     */
    virtual int64_t getTotalStudentCount() const = 0;

    /**
     * @brief Get total active students
     * @return Active student count
     */
    virtual int64_t getActiveStudentCount() const = 0;

    /**
     * @brief Add emergency contact info
     * @param studentId Student ID
     * @param contactName Contact name
     * @param contactPhone Contact phone
     * @throws std::runtime_error on failure
     */
    virtual void setEmergencyContact(
        int64_t studentId,
        const std::string& contactName,
        const std::string& contactPhone
    ) = 0;

    /**
     * @brief Add guardian information
     * @param studentId Student ID
     * @param guardianName Guardian name
     * @param guardianPhone Guardian phone
     * @param guardianEmail Guardian email
     * @throws std::runtime_error on failure
     */
    virtual void setGuardianInfo(
        int64_t studentId,
        const std::string& guardianName,
        const std::string& guardianPhone,
        const std::string& guardianEmail
    ) = 0;
};

} // namespace sms::app::students::services
