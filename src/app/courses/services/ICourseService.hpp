#pragma once

#include "../domain/Entities.hpp"
#include "../repositories/ICourseRepository.hpp"
#include <optional>
#include <vector>
#include <memory>
#include <string>

namespace sms::app::courses::services {

using namespace sms::app::courses::domain;
using namespace sms::app::courses::repositories;

/**
 * @brief Course service interface
 *
 * High-level business operations for course management
 */
class ICourseService {
public:
    virtual ~ICourseService() = default;

    /**
     * @brief Create a new course offering
     * @param code Unique course code
     * @param title Course title
     * @param credits Credit units
     * @param deptId Department ID
     * @return Created course
     * @throws std::runtime_error on validation failure
     */
    virtual Course createCourse(
        const std::string& code,
        const std::string& title,
        int credits,
        int64_t deptId
    ) = 0;

    /**
     * @brief Get course by ID
     * @param courseId Course ID
     * @return Course if found, std::nullopt otherwise
     */
    virtual std::optional<Course> getCourse(int64_t courseId) const = 0;

    /**
     * @brief Get course by code
     * @param code Course code
     * @return Course if found, std::nullopt otherwise
     */
    virtual std::optional<Course> getCourseByCode(const std::string& code) const = 0;

    /**
     * @brief Update course information
     * @param course Course with updated data
     * @return Updated course
     * @throws std::runtime_error on failure
     */
    virtual Course updateCourse(const Course& course) = 0;

    /**
     * @brief Assign a lecturer to a course
     * @param courseId Course ID
     * @param lecturerId Lecturer ID
     * @throws std::runtime_error on failure
     */
    virtual void assignLecturer(int64_t courseId, int64_t lecturerId) = 0;

    /**
     * @brief Set course capacity
     * @param courseId Course ID
     * @param capacity New capacity
     * @throws std::runtime_error on failure
     */
    virtual void setCourseCapacity(int64_t courseId, int capacity) = 0;

    /**
     * @brief Archive a course
     * @param courseId Course ID
     * @throws std::runtime_error on failure
     */
    virtual void archiveCourse(int64_t courseId) = 0;

    /**
     * @brief Add a prerequisite for a course
     * @param courseId Course that has the prerequisite
     * @param prereqCourseId Required course ID
     * @throws std::runtime_error on failure
     */
    virtual void addPrerequisite(int64_t courseId, int64_t prereqCourseId) = 0;

    /**
     * @brief Get all prerequisites for a course
     * @param courseId Course ID
     * @return Vector of prerequisites
     */
    virtual std::vector<CoursePrerequisite> getPrerequisites(int64_t courseId) const = 0;

    /**
     * @brief Search courses by title or code
     * @param searchTerm Search term
     * @return Vector of matching courses
     */
    virtual std::vector<Course> searchCourses(const std::string& searchTerm) const = 0;

    /**
     * @brief Get all active courses for a semester/year
     * @param semester Semester enum
     * @param year Academic year
     * @return Vector of active courses
     */
    virtual std::vector<Course> getActiveCourses(Semester semester, int year) const = 0;

    /**
     * @brief Get courses for a specific department
     * @param deptId Department ID
     * @return Vector of courses
     */
    virtual std::vector<Course> getCoursesByDepartment(int64_t deptId) const = 0;

    /**
     * @brief Get courses taught by a lecturer
     * @param lecturerId Lecturer ID
     * @return Vector of courses
     */
    virtual std::vector<Course> getCoursesByLecturer(int64_t lecturerId) const = 0;

    /**
     * @brief Get total number of courses
     * @return Course count
     */
    virtual int64_t getTotalCourseCount() const = 0;
};

} // namespace sms::app::courses::services
