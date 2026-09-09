#pragma once

#include "IAttendanceRepository.hpp"
#include "database/ConnectionPool.hpp"
#include <pqxx/pqxx>
#include <memory>

namespace sms::app::attendance::repositories {

class PostgreSQLAttendanceRepository : public IAttendanceRepository {
public:
    explicit PostgreSQLAttendanceRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    // IRepository methods
    AttendanceRecord save(const AttendanceRecord& record) override;
    std::optional<AttendanceRecord> findById(int64_t id) const override;
    bool remove(int64_t id) override;
    int64_t count() const override;

    // IAttendanceRepository methods
    std::optional<AttendanceRecord> findByStudentCourseDate(
        int64_t studentId,
        int64_t courseId,
        const std::string& sessionDate
    ) const override;

    std::vector<AttendanceRecord> findByStudent(int64_t studentId) const override;
    std::vector<AttendanceRecord> findByCourse(int64_t courseId) const override;
    std::vector<AttendanceRecord> findByDate(const std::string& sessionDate) const override;
    double getAttendancePercentage(int64_t studentId, int64_t courseId) const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;
    AttendanceRecord mapRowToRecord(const pqxx::row& row) const;
};

} // namespace sms::app::attendance::repositories
