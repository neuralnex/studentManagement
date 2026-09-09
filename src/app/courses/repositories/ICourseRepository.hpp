#pragma once

#include "../domain/Entities.hpp"
#include "database/repositories/IRepository.hpp"
#include <optional>
#include <vector>

namespace sms::app::courses::repositories {

using namespace sms::app::courses::domain;

/**
 * @brief Course repository interface
 */
class ICourseRepository : public sms::database::repositories::IRepository<Course, int64_t> {
public:
    virtual ~ICourseRepository() = default;

    /**
     * @brief Find course by code
     * @param code Course code (e.g., "CS101")
     * @return Course if found, std::nullopt otherwise
     */
    virtual std::optional<Course> findByCode(const std::string& code) const = 0;

    /**
     * @brief Find courses by department
     * @param departmentId Department ID
     * @return Vector of courses in department
     */
    virtual std::vector<Course> findByDepartment(int64_t departmentId) const = 0;

    /**
     * @brief Find courses by lecturer
     * @param lecturerId Lecturer ID
     * @return Vector of courses taught by lecturer
     */
    virtual std::vector<Course> findByLecturer(int64_t lecturerId) const = 0;

    /**
     * @brief Find courses by semester and year
     * @param semester Semester (FALL, SPRING, SUMMER)
     * @param year Academic year
     * @return Vector of courses in that semester
     */
    virtual std::vector<Course> findBySemester(Semester semester, int year) const = 0;

    /**
     * @brief Find active courses
     * @return Vector of active courses
     */
    virtual std::vector<Course> findActive() const = 0;

    /**
     * @brief Find courses with available capacity
     * @return Vector of courses with slots available
     */
    virtual std::vector<Course> findWithAvailableSlots() const = 0;

    /**
     * @brief Search courses by title or code
     * @param searchTerm Search term
     * @return Vector of matching courses
     */
    virtual std::vector<Course> search(const std::string& searchTerm) const = 0;

    /**
     * @brief Get courses by status
     * @param status Course status
     * @return Vector of courses with that status
     */
    virtual std::vector<Course> findByStatus(CourseStatus status) const = 0;

    /**
     * @brief Update enrollment count
     * @param courseId Course ID
     * @param newCount New enrollment count
     * @throws std::runtime_error on failure
     */
    virtual void updateEnrollmentCount(int64_t courseId, int newCount) = 0;

    /**
     * @brief Check if course code exists
     * @param code Course code
     * @return True if exists
     */
    virtual bool codeExists(const std::string& code) const = 0;
};

/**
 * @brief Course prerequisite repository interface
 */
class ICoursePrerequisiteRepository : public sms::database::repositories::IRepository<CoursePrerequisite, int64_t> {
public:
    virtual ~ICoursePrerequisiteRepository() = default;

    /**
     * @brief Get prerequisites for a course
     * @param courseId Course ID
     * @return Vector of prerequisite courses
     */
    virtual std::vector<CoursePrerequisite> getPrerequisites(int64_t courseId) const = 0;

    /**
     * @brief Add prerequisite to course
     * @param courseId Course ID
     * @param prerequisiteId Prerequisite course ID
     * @param minGrade Minimum grade required (optional)
     * @return Created prerequisite
     */
    virtual CoursePrerequisite addPrerequisite(
        int64_t courseId,
        int64_t prerequisiteId,
        const std::string& minGrade = ""
    ) = 0;

    /**
     * @brief Remove prerequisite from course
     * @param courseId Course ID
     * @param prerequisiteId Prerequisite course ID
     * @return True if removed, false if not found
     */
    virtual bool removePrerequisite(int64_t courseId, int64_t prerequisiteId) = 0;

    /**
     * @brief Check if course is prerequisite for another
     * @param courseId Course to check
     * @param targetCourseId Target course
     * @return True if courseId is prerequisite for targetCourseId
     */
    virtual bool isPrerequisiteFor(int64_t courseId, int64_t targetCourseId) const = 0;

    /**
     * @brief Get all courses that require this course as prerequisite
     * @param courseId Course ID
     * @return Vector of courses that require this as prerequisite
     */
    virtual std::vector<CoursePrerequisite> getRequiredByCoursees(int64_t courseId) const = 0;
};

} // namespace sms::app::courses::repositories
