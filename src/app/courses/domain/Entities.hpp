#pragma once

#include <string>
#include <optional>
#include <vector>
#include <chrono>
#include <cstdint>

namespace sms::app::courses::domain {

using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief Course status enumeration
 */
enum class CourseStatus {
    DRAFT,       ///< Being prepared
    ACTIVE,      ///< Currently offered
    ARCHIVED,    ///< No longer offered
    CANCELLED    ///< Cancelled for this semester
};

/**
 * @brief Semester enumeration
 */
enum class Semester {
    FALL,        ///< Fall semester
    SPRING,      ///< Spring semester
    SUMMER       ///< Summer semester
};

/**
 * @brief Course entity
 * 
 * Represents a course offering with credit, prerequisites, and capacity information.
 */
class Course {
public:
    Course() = default;

    /**
     * @brief Constructor with required fields
     * @param id Course ID
     * @param code Course code (e.g., "CS101")
     * @param title Course title
     * @param creditUnits Number of credit units
     */
    Course(int64_t id, const std::string& code, const std::string& title, int creditUnits)
        : m_id(id), m_code(code), m_title(title), m_creditUnits(creditUnits) {}

    // Getters
    [[nodiscard]] int64_t id() const { return m_id; }
    [[nodiscard]] const std::string& code() const { return m_code; }
    [[nodiscard]] const std::string& title() const { return m_title; }
    [[nodiscard]] const std::string& description() const { return m_description; }
    [[nodiscard]] int creditUnits() const { return m_creditUnits; }
    [[nodiscard]] int capacity() const { return m_capacity; }
    [[nodiscard]] int enrolledCount() const { return m_enrolledCount; }
    [[nodiscard]] int64_t departmentId() const { return m_departmentId; }
    [[nodiscard]] int64_t lecturerId() const { return m_lecturerId; }
    [[nodiscard]] CourseStatus status() const { return m_status; }
    [[nodiscard]] Semester semester() const { return m_semester; }
    [[nodiscard]] int year() const { return m_year; }
    [[nodiscard]] const std::string& location() const { return m_location; }
    [[nodiscard]] const std::string& schedule() const { return m_schedule; }
    [[nodiscard]] const TimePoint& startDate() const { return m_startDate; }
    [[nodiscard]] const TimePoint& endDate() const { return m_endDate; }
    [[nodiscard]] const TimePoint& createdAt() const { return m_createdAt; }
    [[nodiscard]] const TimePoint& updatedAt() const { return m_updatedAt; }

    // Setters
    void setId(int64_t id) { m_id = id; }
    void setCode(const std::string& code) { m_code = code; }
    void setTitle(const std::string& title) { m_title = title; }
    void setDescription(const std::string& desc) { m_description = desc; }
    void setCreditUnits(int credits) { m_creditUnits = credits; }
    void setCapacity(int cap) { m_capacity = cap; }
    void setEnrolledCount(int count) { m_enrolledCount = count; }
    void setDepartmentId(int64_t deptId) { m_departmentId = deptId; }
    void setLecturerId(int64_t lectId) { m_lecturerId = lectId; }
    void setStatus(CourseStatus status) { m_status = status; }
    void setSemester(Semester sem) { m_semester = sem; }
    void setYear(int y) { m_year = y; }
    void setLocation(const std::string& loc) { m_location = loc; }
    void setSchedule(const std::string& sched) { m_schedule = sched; }
    void setStartDate(const TimePoint& date) { m_startDate = date; }
    void setEndDate(const TimePoint& date) { m_endDate = date; }

    /**
     * @brief Check if course is active
     * @return True if status is ACTIVE
     */
    [[nodiscard]] bool isActive() const {
        return m_status == CourseStatus::ACTIVE;
    }

    /**
     * @brief Check if course has available slots
     * @return True if enrolled < capacity
     */
    [[nodiscard]] bool hasAvailableSlots() const {
        return m_enrolledCount < m_capacity;
    }

    /**
     * @brief Get available slots count
     * @return Number of available slots
     */
    [[nodiscard]] int getAvailableSlots() const {
        return m_capacity - m_enrolledCount;
    }

    /**
     * @brief Get enrollment percentage
     * @return Enrollment percentage (0-100)
     */
    [[nodiscard]] double getEnrollmentPercentage() const {
        if (m_capacity == 0) return 0.0;
        return (static_cast<double>(m_enrolledCount) / m_capacity) * 100.0;
    }

    bool operator==(const Course& other) const {
        return m_id == other.m_id;
    }

private:
    int64_t m_id{0};
    std::string m_code;
    std::string m_title;
    std::string m_description;
    int m_creditUnits{0};
    int m_capacity{0};
    int m_enrolledCount{0};
    int64_t m_departmentId{0};
    int64_t m_lecturerId{0};
    CourseStatus m_status{CourseStatus::DRAFT};
    Semester m_semester{Semester::FALL};
    int m_year{0};
    std::string m_location;
    std::string m_schedule;
    TimePoint m_startDate;
    TimePoint m_endDate;
    TimePoint m_createdAt;
    TimePoint m_updatedAt;
};

/**
 * @brief Course prerequisite entity
 * 
 * Represents a prerequisite requirement for a course.
 */
class CoursePrerequisite {
public:
    CoursePrerequisite() = default;

    /**
     * @brief Constructor
     * @param courseId Course that has the prerequisite
     * @param prerequisiteCourseId Required course
     * @param minGrade Minimum grade required (optional, e.g., "C")
     */
    CoursePrerequisite(int64_t courseId, int64_t prerequisiteCourseId, const std::string& minGrade = "")
        : m_courseId(courseId), m_prerequisiteCourseId(prerequisiteCourseId), m_minGrade(minGrade) {}

    [[nodiscard]] int64_t courseId() const { return m_courseId; }
    [[nodiscard]] int64_t prerequisiteCourseId() const { return m_prerequisiteCourseId; }
    [[nodiscard]] const std::string& minGrade() const { return m_minGrade; }

    void setCourseId(int64_t id) { m_courseId = id; }
    void setPrerequisiteCourseId(int64_t id) { m_prerequisiteCourseId = id; }
    void setMinGrade(const std::string& grade) { m_minGrade = grade; }

private:
    int64_t m_courseId{0};
    int64_t m_prerequisiteCourseId{0};
    std::string m_minGrade;
};

/**
 * @brief Convert status enum to string
 * @param status Course status
 * @return String representation
 */
inline std::string statusToString(CourseStatus status) {
    switch (status) {
        case CourseStatus::DRAFT: return "DRAFT";
        case CourseStatus::ACTIVE: return "ACTIVE";
        case CourseStatus::ARCHIVED: return "ARCHIVED";
        case CourseStatus::CANCELLED: return "CANCELLED";
    }
    return "DRAFT";
}

/**
 * @brief Convert string to status enum
 * @param statusStr String representation
 * @return Status enum
 */
inline CourseStatus stringToStatus(const std::string& statusStr) {
    if (statusStr == "DRAFT") return CourseStatus::DRAFT;
    if (statusStr == "ACTIVE") return CourseStatus::ACTIVE;
    if (statusStr == "ARCHIVED") return CourseStatus::ARCHIVED;
    if (statusStr == "CANCELLED") return CourseStatus::CANCELLED;
    return CourseStatus::DRAFT;
}

/**
 * @brief Convert semester enum to string
 * @param semester Semester
 * @return String representation
 */
inline std::string semesterToString(Semester semester) {
    switch (semester) {
        case Semester::FALL: return "FALL";
        case Semester::SPRING: return "SPRING";
        case Semester::SUMMER: return "SUMMER";
    }
    return "FALL";
}

/**
 * @brief Convert string to semester enum
 * @param semStr String representation
 * @return Semester enum
 */
inline Semester stringToSemester(const std::string& semStr) {
    if (semStr == "FALL") return Semester::FALL;
    if (semStr == "SPRING") return Semester::SPRING;
    if (semStr == "SUMMER") return Semester::SUMMER;
    return Semester::FALL;
}

} // namespace sms::app::courses::domain
