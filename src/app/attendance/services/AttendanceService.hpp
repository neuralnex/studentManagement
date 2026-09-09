#pragma once

#include "IAttendanceService.hpp"
#include <memory>

namespace sms::app::attendance::services {

class AttendanceService : public IAttendanceService {
public:
    explicit AttendanceService(std::shared_ptr<IAttendanceRepository> attendanceRepo);

    AttendanceRecord recordAttendance(
        int64_t studentId,
        int64_t courseId,
        const std::string& sessionDate,
        AttendanceStatus status,
        int64_t recorderId
    ) override;

    void markAllPresent(int64_t courseId, const std::string& sessionDate, int64_t recorderId) override;
    AttendanceRecord updateAttendance(const AttendanceRecord& record) override;
    std::vector<AttendanceRecord> getStudentAttendance(int64_t studentId, int64_t courseId) const override;
    std::vector<AttendanceRecord> getCourseAttendance(int64_t courseId, const std::string& sessionDate) const override;
    double getAttendancePercentage(int64_t studentId, int64_t courseId) const override;
    std::vector<int64_t> getStudentsWithLowAttendance(int64_t courseId, double threshold) const override;

private:
    std::shared_ptr<IAttendanceRepository> m_attendanceRepo;
};

} // namespace sms::app::attendance::services
