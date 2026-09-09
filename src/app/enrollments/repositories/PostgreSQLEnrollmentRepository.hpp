#pragma once

#include "IEnrollmentRepository.hpp"
#include "database/ConnectionPool.hpp"
#include <memory>

namespace sms::app::enrollments::repositories {

/**
 * @brief PostgreSQL implementation of IEnrollmentRepository
 */
class PostgreSQLEnrollmentRepository : public IEnrollmentRepository {
public:
    explicit PostgreSQLEnrollmentRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    Enrollment create(const Enrollment& entity) override;
    std::optional<Enrollment> read(int64_t id) const override;
    Enrollment update(const Enrollment& entity) override;
    bool delete_by_id(int64_t id) override;
    std::vector<Enrollment> readAll() const override;
    std::vector<Enrollment> readPage(int64_t offset, int64_t limit) const override;
    int64_t count() const override;
    bool exists(int64_t id) const override;

    std::vector<Enrollment> findByStudent(int64_t studentId) const override;
    std::vector<Enrollment> findByCourse(int64_t courseId) const override;
    std::optional<Enrollment> findByStudentAndCourse(int64_t studentId, int64_t courseId) const override;
    std::vector<Enrollment> findBySemester(const std::string& semester) const override;
    std::vector<Enrollment> findByStatus(EnrollmentStatus status) const override;
    bool isEnrolled(int64_t studentId, int64_t courseId) const override;
    int64_t countActiveEnrollments(int64_t courseId) const override;
    double calculateStudentGPA(int64_t studentId) const override;
    void updateStatus(int64_t enrollmentId, EnrollmentStatus status) override;
    void postGrade(int64_t enrollmentId, double numericalGrade, const std::string& letterGrade) override;
    std::vector<Enrollment> getUngradedEnrollments() const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;

    static Enrollment mapRowToEnrollment(const pqxx::row_ref& row);
    static std::string gradeToLetter(double numericGrade);
    static double letterToGPA(const std::string& letterGrade);
};

} // namespace sms::app::enrollments::repositories
