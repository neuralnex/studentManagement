#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace sms::app::attendance::domain {

/**
 * @brief Attendance status enumeration
 */
enum class AttendanceStatus {
    PRESENT,
    ABSENT,
    EXCUSED
};

/**
 * @brief Attendance record entity
 */
class AttendanceRecord {
public:
    AttendanceRecord() = default;

    AttendanceRecord(int64_t id, int64_t studentId, int64_t courseId, const std::string& sessionDate, AttendanceStatus status)
        : m_id(id), m_studentId(studentId), m_courseId(courseId), m_sessionDate(sessionDate), m_status(status) {}

    // Getters
    [[nodiscard]] int64_t id() const { return m_id; }
    [[nodiscard]] int64_t studentId() const { return m_studentId; }
    [[nodiscard]] int64_t courseId() const { return m_courseId; }
    [[nodiscard]] const std::string& sessionDate() const { return m_sessionDate; }
    [[nodiscard]] AttendanceStatus status() const { return m_status; }
    [[nodiscard]] int64_t recordedBy() const { return m_recordedBy; }

    // Setters
    void setId(int64_t id) { m_id = id; }
    void setStudentId(int64_t id) { m_studentId = id; }
    void setCourseId(int64_t id) { m_courseId = id; }
    void setSessionDate(const std::string& date) { m_sessionDate = date; }
    void setStatus(AttendanceStatus status) { m_status = status; }
    void setRecordedBy(int64_t userId) { m_recordedBy = userId; }

    bool operator==(const AttendanceRecord& other) const {
        return m_id == other.m_id;
    }

private:
    int64_t m_id{0};
    int64_t m_studentId{0};
    int64_t m_courseId{0};
    std::string m_sessionDate;
    AttendanceStatus m_status{AttendanceStatus::ABSENT};
    int64_t m_recordedBy{0};
};

inline std::string statusToString(AttendanceStatus status) {
    switch (status) {
        case AttendanceStatus::PRESENT: return "PRESENT";
        case AttendanceStatus::ABSENT: return "ABSENT";
        case AttendanceStatus::EXCUSED: return "EXCUSED";
    }
    return "ABSENT";
}

inline AttendanceStatus stringToStatus(const std::string& statusStr) {
    if (statusStr == "PRESENT") return AttendanceStatus::PRESENT;
    if (statusStr == "EXCUSED") return AttendanceStatus::EXCUSED;
    return AttendanceStatus::ABSENT;
}

} // namespace sms::app::attendance::domain
