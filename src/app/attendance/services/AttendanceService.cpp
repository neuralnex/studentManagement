#include "AttendanceService.hpp"
#include <stdexcept>
#include <algorithm>

namespace sms::app::attendance::services {

AttendanceService::AttendanceService(std::shared_ptr<IAttendanceRepository> attendanceRepo)
    : m_attendanceRepo(attendanceRepo) {}

AttendanceRecord AttendanceService::recordAttendance(
    int64_t studentId,
    int64_t courseId,
    const std::string& sessionDate,
    AttendanceStatus status,
    int64_t recorderId
) {
    AttendanceRecord record;
    record.setStudentId(studentId);
    record.setCourseId(courseId);
    record.setSessionDate(sessionDate);
    record.setStatus(status);
    record.setRecordedBy(recorderId);

    return m_attendanceRepo->save(record);
}

void AttendanceService::markAllPresent(int64_t courseId, const std::string& sessionDate, int64_t recorderId) {
    auto records = m_attendanceRepo->findByCourse(courseId);
    // This is a bit simplistic, usually you'd get all students enrolled in the course
    // But for this implementation, we'll just update existing records for that date
    for (auto& record : records) {
        if (record.sessionDate() == sessionDate) {
            record.setStatus(AttendanceStatus::PRESENT);
            record.setRecordedBy(recorderId);
            m_attendanceRepo->save(record);
        }
    }
}

AttendanceRecord AttendanceService::updateAttendance(const AttendanceRecord& record) {
    return m_attendanceRepo->save(record);
}

std::vector<AttendanceRecord> AttendanceService::getStudentAttendance(int64_t studentId, int64_t courseId) const {
    auto allStudentRecords = m_attendanceRepo->findByStudent(studentId);
    std::vector<AttendanceRecord> courseRecords;

    std::copy_if(allStudentRecords.begin(), allStudentRecords.end(), std::back_inserter(courseRecords),
        [courseId](const AttendanceRecord& r) { return r.courseId() == courseId; });

    return courseRecords;
}

std::vector<AttendanceRecord> AttendanceService::getCourseAttendance(int64_t courseId, const std::string& sessionDate) const {
    auto courseRecords = m_attendanceRepo->findByCourse(courseId);
    std::vector<AttendanceRecord> dateRecords;

    std::copy_if(courseRecords.begin(), courseRecords.end(), std::back_inserter(dateRecords),
        [&sessionDate](const AttendanceRecord& r) { return r.sessionDate() == sessionDate; });

    return dateRecords;
}

double AttendanceService::getAttendancePercentage(int64_t studentId, int64_t courseId) const {
    return m_attendanceRepo->getAttendancePercentage(studentId, courseId);
}

std::vector<int64_t> AttendanceService::getStudentsWithLowAttendance(int64_t courseId, double threshold) const {
    auto records = m_attendanceRepo->findByCourse(courseId);
    std::vector<int64_t> lowAttendanceStudents;

    // This is a bit inefficient, ideally would be a DB query
    std::vector<int64_t> studentIds;
    for (const auto& r : records) studentIds.push_back(r.studentId());

    // Remove duplicates
    std::sort(studentIds.begin(), studentIds.end());
    studentIds.erase(std::unique(studentIds.begin(), studentIds.end()), studentIds.end());

    for (auto id : studentIds) {
        if (getAttendancePercentage(id, courseId) < threshold) {
            lowAttendanceStudents.push_back(id);
        }
    }

    return lowAttendanceStudents;
}

} // namespace sms::app::attendance::services
