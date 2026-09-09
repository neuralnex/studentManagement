#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <cstdint>

namespace sms::app::enrollments::domain {

using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief Enrollment status enumeration
 */
enum class EnrollmentStatus {
    PENDING,        ///< Awaiting approval
    APPROVED,       ///< Approved by advisor
    ACTIVE,         ///< Currently enrolled
    COMPLETED,      ///< Completed the course
    DROPPED,        ///< Student dropped course
    WITHDRAWN,      ///< Student withdrew (after deadline)
    FAILED          ///< Did not complete course
};

/**
 * @brief Enrollment entity
 * 
 * Represents a student's enrollment in a course
 */
class Enrollment {
public:
    Enrollment() = default;

    /**
     * @brief Constructor with required fields
     * @param id Enrollment ID
     * @param studentId Student ID
     * @param courseId Course ID
     */
    Enrollment(int64_t id, int64_t studentId, int64_t courseId)
        : m_id(id), m_studentId(studentId), m_courseId(courseId) {}

    // Getters
    [[nodiscard]] int64_t id() const { return m_id; }
    [[nodiscard]] int64_t studentId() const { return m_studentId; }
    [[nodiscard]] int64_t courseId() const { return m_courseId; }
    [[nodiscard]] EnrollmentStatus status() const { return m_status; }
    [[nodiscard]] const std::string& semester() const { return m_semester; }
    [[nodiscard]] int year() const { return m_year; }
    [[nodiscard]] double grade() const { return m_grade; }
    [[nodiscard]] bool gradePosted() const { return m_gradePosted; }
    [[nodiscard]] const std::string& letterGrade() const { return m_letterGrade; }
    [[nodiscard]] double gpa() const { return m_gpa; }
    [[nodiscard]] int creditHours() const { return m_creditHours; }
    [[nodiscard]] bool prerequisitesMet() const { return m_prerequisitesMet; }
    [[nodiscard]] const TimePoint& enrolledDate() const { return m_enrolledDate; }
    [[nodiscard]] const TimePoint& completedDate() const { return m_completedDate; }
    [[nodiscard]] const TimePoint& createdAt() const { return m_createdAt; }
    [[nodiscard]] const TimePoint& updatedAt() const { return m_updatedAt; }

    // Setters
    void setId(int64_t id) { m_id = id; }
    void setStudentId(int64_t id) { m_studentId = id; }
    void setCourseId(int64_t id) { m_courseId = id; }
    void setStatus(EnrollmentStatus status) { m_status = status; }
    void setSemester(const std::string& sem) { m_semester = sem; }
    void setYear(int y) { m_year = y; }
    void setGrade(double g) { m_grade = g; }
    void setGradePosted(bool posted) { m_gradePosted = posted; }
    void setLetterGrade(const std::string& grade) { m_letterGrade = grade; }
    void setGPA(double gpa) { m_gpa = gpa; }
    void setCreditHours(int hours) { m_creditHours = hours; }
    void setPrerequisitesMet(bool met) { m_prerequisitesMet = met; }
    void setEnrolledDate(const TimePoint& date) { m_enrolledDate = date; }
    void setCompletedDate(const TimePoint& date) { m_completedDate = date; }

    /**
     * @brief Check if enrollment is active
     * @return True if status is ACTIVE
     */
    [[nodiscard]] bool isActive() const {
        return m_status == EnrollmentStatus::ACTIVE;
    }

    /**
     * @brief Check if enrollment is completed
     * @return True if status is COMPLETED or FAILED
     */
    [[nodiscard]] bool isCompleted() const {
        return m_status == EnrollmentStatus::COMPLETED || m_status == EnrollmentStatus::FAILED;
    }

    /**
     * @brief Check if grade can be posted
     * @return True if completed and not yet posted
     */
    [[nodiscard]] bool canPostGrade() const {
        return isCompleted() && !m_gradePosted;
    }

    bool operator==(const Enrollment& other) const {
        return m_id == other.m_id;
    }

private:
    int64_t m_id{0};
    int64_t m_studentId{0};
    int64_t m_courseId{0};
    EnrollmentStatus m_status{EnrollmentStatus::PENDING};
    std::string m_semester;
    int m_year{0};
    double m_grade{0.0};
    bool m_gradePosted{false};
    std::string m_letterGrade;
    double m_gpa{0.0};
    int m_creditHours{0};
    bool m_prerequisitesMet{true};
    TimePoint m_enrolledDate;
    TimePoint m_completedDate;
    TimePoint m_createdAt;
    TimePoint m_updatedAt;
};

/**
 * @brief Convert status enum to string
 * @param status Enrollment status
 * @return String representation
 */
inline std::string statusToString(EnrollmentStatus status) {
    switch (status) {
        case EnrollmentStatus::PENDING: return "PENDING";
        case EnrollmentStatus::APPROVED: return "APPROVED";
        case EnrollmentStatus::ACTIVE: return "ACTIVE";
        case EnrollmentStatus::COMPLETED: return "COMPLETED";
        case EnrollmentStatus::DROPPED: return "DROPPED";
        case EnrollmentStatus::WITHDRAWN: return "WITHDRAWN";
        case EnrollmentStatus::FAILED: return "FAILED";
    }
    return "PENDING";
}

/**
 * @brief Convert string to status enum
 * @param statusStr String representation
 * @return Status enum
 */
inline EnrollmentStatus stringToStatus(const std::string& statusStr) {
    if (statusStr == "PENDING") return EnrollmentStatus::PENDING;
    if (statusStr == "APPROVED") return EnrollmentStatus::APPROVED;
    if (statusStr == "ACTIVE") return EnrollmentStatus::ACTIVE;
    if (statusStr == "COMPLETED") return EnrollmentStatus::COMPLETED;
    if (statusStr == "DROPPED") return EnrollmentStatus::DROPPED;
    if (statusStr == "WITHDRAWN") return EnrollmentStatus::WITHDRAWN;
    if (statusStr == "FAILED") return EnrollmentStatus::FAILED;
    return EnrollmentStatus::PENDING;
}

} // namespace sms::app::enrollments::domain
