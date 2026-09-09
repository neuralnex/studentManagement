#pragma once

#include "../domain/Entities.hpp"
#include "../repositories/IEnrollmentRepository.hpp"
#include "../../courses/repositories/ICourseRepository.hpp"
#include "../../students/repositories/IStudentRepository.hpp"
#include <optional>
#include <vector>
#include <memory>

namespace sms::app::enrollments::services {

using namespace sms::app::enrollments::domain;
using namespace sms::app::enrollments::repositories;

/**
 * @brief Enrollment service interface
 */
class IEnrollmentService {
public:
    virtual ~IEnrollmentService() = default;

    /**
     * @brief Enroll student in course
     * @param studentId Student ID
     * @param courseId Course ID
     * @param semester Semester (e.g., "FALL2026")
     * @param year Academic year
     * @return Created enrollment
     * @throws std::runtime_error on validation failure
     */
    virtual Enrollment enrollStudent(
        int64_t studentId,
        int64_t courseId,
        const std::string& semester,
        int year
    ) = 0;

    /**
     * @brief Get student's enrollments
     * @param studentId Student ID
     * @return Vector of student's enrollments
     */
    virtual std::vector<Enrollment> getStudentEnrollments(int64_t studentId) const = 0;

    /**
     * @brief Get course enrollments
     * @param courseId Course ID
     * @return Vector of course's enrollments
     */
    virtual std::vector<Enrollment> getCourseEnrollments(int64_t courseId) const = 0;

    /**
     * @brief Drop course enrollment
     * @param enrollmentId Enrollment ID
     * @throws std::runtime_error on failure
     */
    virtual void dropCourse(int64_t enrollmentId) = 0;

    /**
     * @brief Withdraw from course (after drop deadline)
     * @param enrollmentId Enrollment ID
     * @throws std::runtime_error on failure
     */
    virtual void withdrawFromCourse(int64_t enrollmentId) = 0;

    /**
     * @brief Approve enrollment
     * @param enrollmentId Enrollment ID
     * @throws std::runtime_error on failure
     */
    virtual void approveEnrollment(int64_t enrollmentId) = 0;

    /**
     * @brief Activate enrollment
     * @param enrollmentId Enrollment ID
     * @throws std::runtime_error on failure
     */
    virtual void activateEnrollment(int64_t enrollmentId) = 0;

    /**
     * @brief Mark enrollment as completed
     * @param enrollmentId Enrollment ID
     * @throws std::runtime_error on failure
     */
    virtual void completeEnrollment(int64_t enrollmentId) = 0;

    /**
     * @brief Post grade for enrollment
     * @param enrollmentId Enrollment ID
     * @param numericalGrade Numerical grade (0-100)
     * @throws std::runtime_error on failure
     */
    virtual void postGrade(int64_t enrollmentId, double numericalGrade) = 0;

    /**
     * @brief Get student's GPA
     * @param studentId Student ID
     * @return Calculated GPA
     */
    virtual double getStudentGPA(int64_t studentId) const = 0;

    /**
     * @brief Get student's transcript
     * @param studentId Student ID
     * @return JSON formatted transcript
     */
    virtual std::string generateTranscript(int64_t studentId) const = 0;

    /**
     * @brief Check if student meets prerequisites for course
     * @param studentId Student ID
     * @param courseId Course ID
     * @return True if prerequisites met
     */
    virtual bool checkPrerequisites(int64_t studentId, int64_t courseId) const = 0;

    /**
     * @brief Get enrollments pending grades
     * @return Vector of ungraded enrollments
     */
    virtual std::vector<Enrollment> getPendingGrades() const = 0;

    /**
     * @brief Get enrollment statistics
     * @param courseId Course ID
     * @return JSON statistics
     */
    virtual std::string getEnrollmentStatistics(int64_t courseId) const = 0;
};

} // namespace sms::app::enrollments::services
