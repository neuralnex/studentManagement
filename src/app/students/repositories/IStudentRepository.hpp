#pragma once

#include "../domain/Entities.hpp"
#include "database/repositories/IRepository.hpp"
#include <optional>
#include <vector>

namespace sms::app::students::repositories {

using namespace sms::app::students::domain;

/**
 * @brief Student repository interface
 * 
 * Extends generic repository with student-specific operations
 */
class IStudentRepository : public sms::database::repositories::IRepository<Student, int64_t> {
public:
    virtual ~IStudentRepository() = default;

    /**
     * @brief Find student by matric number
     * @param matricNumber Unique student identification number
     * @return Student if found, std::nullopt otherwise
     */
    virtual std::optional<Student> findByMatricNumber(const std::string& matricNumber) const = 0;

    /**
     * @brief Find student by user ID
     * @param userId Associated user ID
     * @return Student if found, std::nullopt otherwise
     */
    virtual std::optional<Student> findByUserId(int64_t userId) const = 0;

    /**
     * @brief Find students by status
     * @param status Student status
     * @return Vector of students with that status
     */
    virtual std::vector<Student> findByStatus(StudentStatus status) const = 0;

    /**
     * @brief Find students by academic standing
     * @param standing Academic standing
     * @return Vector of students with that standing
     */
    virtual std::vector<Student> findByAcademicStanding(AcademicStanding standing) const = 0;

    /**
     * @brief Search students by name (first or last)
     * @param name Search term (partial match)
     * @return Vector of matching students
     */
    virtual std::vector<Student> searchByName(const std::string& name) const = 0;

    /**
     * @brief Check if matric number exists
     * @param matricNumber Matric number to check
     * @return True if exists
     */
    virtual bool matricNumberExists(const std::string& matricNumber) const = 0;

    /**
     * @brief Get students in probation status
     * @return Vector of students on probation
     */
    virtual std::vector<Student> getStudentsOnProbation() const = 0;

    /**
     * @brief Get total number of active students
     * @return Count of active students
     */
    virtual int64_t countActiveStudents() const = 0;

    /**
     * @brief Get graduated students
     * @param offset Pagination offset
     * @param limit Pagination limit
     * @return Vector of graduated students
     */
    virtual std::vector<Student> getGraduatedStudents(int64_t offset, int64_t limit) const = 0;

    /**
     * @brief Update student's academic standing
     * @param studentId Student ID
     * @param standing New academic standing
     * @throws std::runtime_error on failure
     */
    virtual void updateAcademicStanding(int64_t studentId, AcademicStanding standing) = 0;

    /**
     * @brief Get students by partial name match
     * @param firstName First name (partial)
     * @param lastName Last name (partial)
     * @return Vector of matching students
     */
    virtual std::vector<Student> findByNameParts(
        const std::string& firstName,
        const std::string& lastName
    ) const = 0;
};

} // namespace sms::app::students::repositories
