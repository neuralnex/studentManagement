#include "PostgreSQLAttendanceRepository.hpp"
#include <stdexcept>
#include <iomanip>
#include <sstream>

namespace sms::app::attendance::repositories {

PostgreSQLAttendanceRepository::PostgreSQLAttendanceRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(pool) {}

AttendanceRecord PostgreSQLAttendanceRepository::save(const AttendanceRecord& record) {
    auto conn = m_pool->getConnection();
    pqxx::work txn(*conn);

    std::string query =
        "INSERT INTO attendance (student_id, course_id, session_date, status, recorded_by) "
        "VALUES ($1, $2, $3, $4, $5) "
        "ON CONFLICT (student_id, course_id, session_date) "
        "DO UPDATE SET status = EXCLUDED.status, recorded_by = EXCLUDED.recorded_by "
        "RETURNING id";

    pqxx::result res = txn.exec_params(query,
        record.studentId(),
        record.courseId(),
        record.sessionDate(),
        statusToString(record.status()),
        record.recordedBy());

    txn.commit();

    int64_t id = res[0][0].as<int64_t>();
    AttendanceRecord savedRecord = record;
    savedRecord.setId(id);
    return savedRecord;
}

std::optional<AttendanceRecord> PostgreSQLAttendanceRepository::findById(int64_t id) const {
    auto conn = m_pool->getConnection();
    pqxx::nontransaction txn(*conn);

    pqxx::result res = txn.exec_params("SELECT * FROM attendance WHERE id = $1", id);

    if (res.empty()) return std::nullopt;
    return mapRowToRecord(res[0]);
}

bool PostgreSQLAttendanceRepository::remove(int64_t id) {
    auto conn = m_pool->getConnection();
    pqxx::work txn(*conn);

    pqxx::result res = txn.exec_params("DELETE FROM attendance WHERE id = $1", id);
    txn.commit();

    return res.affected_rows() > 0;
}

int64_t PostgreSQLAttendanceRepository::count() const {
    auto conn = m_pool->getConnection();
    pqxx::nontransaction txn(*conn);

    pqxx::result res = txn.exec("SELECT count(*) FROM attendance");
    return res[0][0].as<int64_t>();
}

std::optional<AttendanceRecord> PostgreSQLAttendanceRepository::findByStudentCourseDate(
    int64_t studentId,
    int64_t courseId,
    const std::string& sessionDate
) const {
    auto conn = m_pool->getConnection();
    pqxx::nontransaction txn(*conn);

    pqxx::result res = txn.exec_params(
        "SELECT * FROM attendance WHERE student_id = $1 AND course_id = $2 AND session_date = $3",
        studentId, courseId, sessionDate);

    if (res.empty()) return std::nullopt;
    return mapRowToRecord(res[0]);
}

std::vector<AttendanceRecord> PostgreSQLAttendanceRepository::findByStudent(int64_t studentId) const {
    auto conn = m_pool->getConnection();
    pqxx::nontransaction txn(*conn);

    pqxx::result res = txn.exec_params("SELECT * FROM attendance WHERE student_id = $1", studentId);

    std::vector<AttendanceRecord> records;
    for (const auto& row : res) {
        records.push_back(mapRowToRecord(row));
    }
    return records;
}

std::vector<AttendanceRecord> PostgreSQLAttendanceRepository::findByCourse(int64_t courseId) const {
    auto conn = m_pool->getConnection();
    pqxx::nontransaction txn(*conn);

    pqxx::result res = txn.exec_params("SELECT * FROM attendance WHERE course_id = $1", courseId);

    std::vector<AttendanceRecord> records;
    for (const auto& row : res) {
        records.push_back(mapRowToRecord(row));
    }
    return records;
}

std::vector<AttendanceRecord> PostgreSQLAttendanceRepository::findByDate(const std::string& sessionDate) const {
    auto conn = m_pool->getConnection();
    pqxx::nontransaction txn(*conn);

    pqxx::result res = txn.exec_params("SELECT * FROM attendance WHERE session_date = $1", sessionDate);

    std::vector<AttendanceRecord> records;
    for (const auto& row : res) {
        records.push_back(mapRowToRecord(row));
    }
    return records;
}

double PostgreSQLAttendanceRepository::getAttendancePercentage(int64_t studentId, int64_t courseId) const {
    auto conn = m_pool->getConnection();
    pqxx::nontransaction txn(*conn);

    pqxx::result res = txn.exec_params(
        "SELECT count(*) FILTER (WHERE status = 'PRESENT') * 100.0 / count(*)::float "
        "FROM attendance WHERE student_id = $1 AND course_id = $2",
        studentId, courseId);

    if (res.empty() || res[0][0].is_null()) return 0.0;
    return res[0][0].as<double>();
}

AttendanceRecord PostgreSQLAttendanceRepository::mapRowToRecord(const pqxx::row& row) const {
    AttendanceRecord record;
    record.setId(row["id"].as<int64_t>());
    record.setStudentId(row["student_id"].as<int64_t>());
    record.setCourseId(row["course_id"].as<int64_t>());
    record.setSessionDate(row["session_date"].as<std::string>());
    record.setStatus(stringToStatus(row["status"].as<std::string>()));
    record.setRecordedBy(row["recorded_by"].as<int64_t>(0));
    return record;
}

} // namespace sms::app::attendance::repositories
