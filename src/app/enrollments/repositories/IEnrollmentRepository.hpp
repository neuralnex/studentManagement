#pragma once

#include "../domain/Entities.hpp"
#include "database/repositories/IRepository.hpp"
#include <optional>
#include <vector>

namespace sms::app::enrollments::repositories {

using namespace sms::app::enrollments::domain;

/**
 * @brief Enrollment repository interface
 */
class IEnrollmentRepository : public sms::database::repositories::IRepository<Enrollment, int64_t> {
public:
    virtual ~IEnrollmentRepository() = default;

    /**
     * @brief Find enrollments by student
     * @param studentId Student ID
     * @return Vector of student's enrollments
     */
    virtual std::vector<Enrollment> findByStudent(int64_t studentId) const = 0;

    /**
     * @brief Find enrollments by course
     * @param courseId Course ID
     * @return Vector of course's enrollments
     */
    virtual std::vector<Enrollment> findByCourse(int64_t courseId) const = 0;

    /**
     * @brief Find specific student enrollment in course
     * @param studentId Student ID
     * @param courseId Course ID
     * @return Enrollment if found, std::nullopt otherwise
     */
    virtual std::optional<Enrollment> findByStudentAndCourse(int64_t studentId, int64_t courseId) const = 0;

    /**
     * @brief Find enrollments by semester
     * @param semester Semester (e.g., "FALL2026")
     * @return Vector of enrollments in that semester
     */
    virtual std::vector<Enrollment> findBySemester(const std::string& semester) const = 0;

    /**
     * @brief Find enrollments by status
     * @param status Enrollment status
     * @return Vector of enrollments with that status
     */
    virtual std::vector<Enrollment> findByStatus(EnrollmentStatus status) const = 0;

    /**
     * @brief Check if student is enrolled in course
     * @param studentId Student ID
     * @param courseId Course ID
     * @return True if enrolled
     */
    virtual bool isEnrolled(int64_t studentId, int64_t courseId) const = 0;

    /**
     * @brief Count active enrollments in course
     * @param courseId Course ID
     * @return Number of active enrollments
     */
    virtual int64_t countActiveEnrollments(int64_t courseId) const = 0;

    /**
     * @brief Get student's GPA
     * @param studentId Student ID
     * @return Calculated GPA
     */
    virtual double calculateStudentGPA(int64_t studentId) const = 0;

    /**
     * @brief Update enrollment status
     * @param enrollmentId Enrollment ID
     * @param status New status
     * @throws std::runtime_error on failure
     */
    virtual void updateStatus(int64_t enrollmentId, EnrollmentStatus status) = 0;

    /**
     * @brief Post grade for enrollment
     * @param enrollmentId Enrollment ID
     * @param numericalGrade Numerical grade (0-100)
     * @param letterGrade Letter grade (A-F)
     * @throws std::runtime_error on failure
     */
    virtual void postGrade(int64_t enrollmentId, double numericalGrade, const std::string& letterGrade) = 0;

    /**
     * @brief Get enrollments pending grades
     * @return Vector of completed but ungraded enrollments
     */
    virtual std::vector<Enrollment> getUngradedEnrollments() const = 0;
};

} // namespace sms::app::enrollments::repositories
