#pragma once

#include "../domain/Entities.hpp"
#include "database/repositories/IRepository.hpp"
#include <vector>
#include <string>
#include <optional>

namespace sms::app::attendance::repositories {

using namespace sms::app::attendance::domain;

/**
 * @brief Attendance repository interface
 */
class IAttendanceRepository : public sms::database::repositories::IRepository<AttendanceRecord, int64_t> {
public:
    virtual ~IAttendanceRepository() = default;

    /**
     * @brief Find attendance by student and course and date
     */
    virtual std::optional<AttendanceRecord> findByStudentCourseDate(
        int64_t studentId,
        int64_t courseId,
        const std::string& sessionDate
    ) const = 0;

    /**
     * @brief Get all attendance records for a student
     */
    virtual std::vector<AttendanceRecord> findByStudent(int64_t studentId) const = 0;

    /**
     * @brief Get all attendance records for a course
     */
    virtual std::vector<AttendanceRecord> findByCourse(int64_t courseId) const = 0;

    /**
     * @brief Get attendance records for a specific date
     */
    virtual std::vector<AttendanceRecord> findByDate(const std::string& sessionDate) const = 0;

    /**
     * @brief Get attendance summary for a student in a course
     * @return Percentage of presence
     */
    virtual double getAttendancePercentage(int64_t studentId, int64_t courseId) const = 0;
};

} // namespace sms::app::attendance::repositories
