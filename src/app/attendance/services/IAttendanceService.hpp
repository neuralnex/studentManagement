#pragma once

#include "../domain/Entities.hpp"
#include "../repositories/IAttendanceRepository.hpp"
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace sms::app::attendance::services {

using namespace sms::app::attendance::domain;
using namespace sms::app::attendance::repositories;

/**
 * @brief Attendance service interface
 */
class IAttendanceService {
public:
    virtual ~IAttendanceService() = default;

    /**
     * @brief Record attendance for a student
     * @param studentId Student ID
     * @param courseId Course ID
     * @param sessionDate Date of session (YYYY-MM-DD)
     * @param status Attendance status
     * @param recorderId User ID recording the attendance
     * @return The recorded attendance
     */
    virtual AttendanceRecord recordAttendance(
        int64_t studentId,
        int64_t courseId,
        const std::string& sessionDate,
        AttendanceStatus status,
        int64_t recorderId
    ) = 0;

    /**
     * @brief Mark a whole course as present for a date
     * @param courseId Course ID
     * @param sessionDate Date of session
     * @param recorderId User ID recording the attendance
     * @throws std::runtime_error on failure
     */
    virtual void markAllPresent(int64_t courseId, const std::string& sessionDate, int64_t recorderId) = 0;

    /**
     * @brief Update existing attendance record
     * @param record Attendance record with updated status
     * @return Updated record
     */
    virtual AttendanceRecord updateAttendance(const AttendanceRecord& record) = 0;

    /**
     * @brief Get attendance for a student in a course
     * @param studentId Student ID
     * @param courseId Course ID
     * @return Vector of records
     */
    virtual std::vector<AttendanceRecord> getStudentAttendance(int64_t studentId, int64_t courseId) const = 0;

    /**
     * @brief Get attendance for a course on a date
     * @param courseId Course ID
     * @param sessionDate Date of session
     * @return Vector of records
     */
    virtual std::vector<AttendanceRecord> getCourseAttendance(int64_t courseId, const std::string& sessionDate) const = 0;

    /**
     * @brief Get attendance percentage for a student in a course
     * @param studentId Student ID
     * @param courseId Course ID
     * @return Percentage (0-100)
     */
    virtual double getAttendancePercentage(int64_t studentId, int64_t courseId) const = 0;

    /**
     * @brief Get students with low attendance
     * @param courseId Course ID
     * @param threshold Percentage threshold (e.g., 75.0)
     * @return Vector of student IDs
     */
    virtual std::vector<int64_t> getStudentsWithLowAttendance(int64_t courseId, double threshold) const = 0;
};

} // namespace sms::app::attendance::services
