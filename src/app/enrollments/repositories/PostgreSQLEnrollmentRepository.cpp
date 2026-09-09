#include "PostgreSQLEnrollmentRepository.hpp"
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>
#include <cmath>

namespace sms::app::enrollments::repositories {

PostgreSQLEnrollmentRepository::PostgreSQLEnrollmentRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(std::move(pool)) {
    if (!m_pool) {
        throw std::invalid_argument("Connection pool cannot be null");
    }
}

std::string PostgreSQLEnrollmentRepository::gradeToLetter(double numericGrade) {
    if (numericGrade >= 90.0) return "A";
    if (numericGrade >= 80.0) return "B";
    if (numericGrade >= 70.0) return "C";
    if (numericGrade >= 60.0) return "D";
    return "F";
}

double PostgreSQLEnrollmentRepository::letterToGPA(const std::string& letterGrade) {
    if (letterGrade == "A") return 4.0;
    if (letterGrade == "B") return 3.0;
    if (letterGrade == "C") return 2.0;
    if (letterGrade == "D") return 1.0;
    if (letterGrade == "F") return 0.0;
    return 0.0;
}

Enrollment PostgreSQLEnrollmentRepository::mapRowToEnrollment(const pqxx::row_ref& row) {
    Enrollment enrollment(
        row["id"].as<int64_t>(),
        row["student_id"].as<int64_t>(),
        row["course_id"].as<int64_t>()
    );

    enrollment.setStatus(stringToStatus(row["status"].as<std::string>()));
    enrollment.setSemester(row["semester"].as<std::string>(""));
    enrollment.setYear(row["year"].as<int>());
    enrollment.setGrade(row["grade"].as<double>(0.0));
    enrollment.setGradePosted(row["grade_posted"].as<bool>(false));
    enrollment.setLetterGrade(row["letter_grade"].as<std::string>(""));
    enrollment.setGPA(row["gpa"].as<double>(0.0));
    enrollment.setCreditHours(row["credit_hours"].as<int>());
    enrollment.setPrerequisitesMet(row["prerequisites_met"].as<bool>(true));

    return enrollment;
}

Enrollment PostgreSQLEnrollmentRepository::create(const Enrollment& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                INSERT INTO enrollments 
                (student_id, course_id, status, semester, year, credit_hours, prerequisites_met, created_at, updated_at)
                VALUES ($1, $2, $3, $4, $5, $6, $7, NOW(), NOW())
                RETURNING id, student_id, course_id, status, semester, year, grade, grade_posted,
                          letter_grade, gpa, credit_hours, prerequisites_met
            )",
            pqxx::params{
            entity.studentId(),
            entity.courseId(),
            statusToString(entity.status()),
            entity.semester(),
            entity.year(),
            entity.creditHours(),
            entity.prerequisitesMet()}
        );

        txn.commit();

        Enrollment created = mapRowToEnrollment(result[0]);
        spdlog::info("Enrollment created: student {} in course {}", entity.studentId(), entity.courseId());
        return created;

    } catch (const std::exception& e) {
        spdlog::error("Failed to create enrollment: {}", e.what());
        throw std::runtime_error(std::string("Failed to create enrollment: ") + e.what());
    }
}

std::optional<Enrollment> PostgreSQLEnrollmentRepository::read(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                WHERE id = $1
            )",
            pqxx::params{
            id}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToEnrollment(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to read enrollment {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to read enrollment: ") + e.what());
    }
}

Enrollment PostgreSQLEnrollmentRepository::update(const Enrollment& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                UPDATE enrollments
                SET student_id = $2, course_id = $3, status = $4, semester = $5, year = $6,
                    grade = $7, grade_posted = $8, letter_grade = $9, gpa = $10, credit_hours = $11,
                    prerequisites_met = $12, updated_at = NOW()
                WHERE id = $1
                RETURNING id, student_id, course_id, status, semester, year, grade, grade_posted,
                          letter_grade, gpa, credit_hours, prerequisites_met
            )",
            pqxx::params{
            entity.id(),
            entity.studentId(),
            entity.courseId(),
            statusToString(entity.status()),
            entity.semester(),
            entity.year(),
            entity.grade(),
            entity.gradePosted(),
            entity.letterGrade(),
            entity.gpa(),
            entity.creditHours(),
            entity.prerequisitesMet()}
        );

        if (result.empty()) {
            throw std::runtime_error("Enrollment not found");
        }

        txn.commit();

        spdlog::info("Enrollment updated: (id={})", entity.id());
        return mapRowToEnrollment(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to update enrollment {}: {}", entity.id(), e.what());
        throw std::runtime_error(std::string("Failed to update enrollment: ") + e.what());
    }
}

bool PostgreSQLEnrollmentRepository::delete_by_id(int64_t id) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"DELETE FROM enrollments WHERE id = $1",
            pqxx::params{
            id}
        );

        txn.commit();

        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete enrollment {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to delete enrollment: ") + e.what());
    }
}

std::vector<Enrollment> PostgreSQLEnrollmentRepository::readAll() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                ORDER BY semester DESC, student_id
            )"
        );

        std::vector<Enrollment> enrollments;
        for (auto row : result) {
            enrollments.push_back(mapRowToEnrollment(row));
        }

        return enrollments;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read all enrollments: {}", e.what());
        throw std::runtime_error(std::string("Failed to read enrollments: ") + e.what());
    }
}

std::vector<Enrollment> PostgreSQLEnrollmentRepository::readPage(int64_t offset, int64_t limit) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                ORDER BY semester DESC, student_id
                LIMIT $1 OFFSET $2
            )",
            pqxx::params{
            limit, offset}
        );

        std::vector<Enrollment> enrollments;
        for (auto row : result) {
            enrollments.push_back(mapRowToEnrollment(row));
        }

        return enrollments;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read enrollment page: {}", e.what());
        throw std::runtime_error(std::string("Failed to read enrollments: ") + e.what());
    }
}

int64_t PostgreSQLEnrollmentRepository::count() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec("SELECT COUNT(*) as count FROM enrollments");

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count enrollments: {}", e.what());
        throw std::runtime_error(std::string("Failed to count enrollments: ") + e.what());
    }
}

bool PostgreSQLEnrollmentRepository::exists(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT 1 FROM enrollments WHERE id = $1",
            pqxx::params{
            id}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check enrollment existence: {}", e.what());
        throw std::runtime_error(std::string("Failed to check existence: ") + e.what());
    }
}

std::vector<Enrollment> PostgreSQLEnrollmentRepository::findByStudent(int64_t studentId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                WHERE student_id = $1
                ORDER BY semester DESC
            )",
            pqxx::params{
            studentId}
        );

        std::vector<Enrollment> enrollments;
        for (auto row : result) {
            enrollments.push_back(mapRowToEnrollment(row));
        }

        return enrollments;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find enrollments for student: {}", e.what());
        throw std::runtime_error(std::string("Failed to find enrollments: ") + e.what());
    }
}

std::vector<Enrollment> PostgreSQLEnrollmentRepository::findByCourse(int64_t courseId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                WHERE course_id = $1
                ORDER BY student_id
            )",
            pqxx::params{
            courseId}
        );

        std::vector<Enrollment> enrollments;
        for (auto row : result) {
            enrollments.push_back(mapRowToEnrollment(row));
        }

        return enrollments;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find enrollments for course: {}", e.what());
        throw std::runtime_error(std::string("Failed to find enrollments: ") + e.what());
    }
}

std::optional<Enrollment> PostgreSQLEnrollmentRepository::findByStudentAndCourse(int64_t studentId, int64_t courseId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                WHERE student_id = $1 AND course_id = $2
            )",
            pqxx::params{
            studentId,
            courseId}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToEnrollment(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to find enrollment: {}", e.what());
        throw std::runtime_error(std::string("Failed to find enrollment: ") + e.what());
    }
}

std::vector<Enrollment> PostgreSQLEnrollmentRepository::findBySemester(const std::string& semester) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                WHERE semester = $1
                ORDER BY student_id
            )",
            pqxx::params{
            semester}
        );

        std::vector<Enrollment> enrollments;
        for (auto row : result) {
            enrollments.push_back(mapRowToEnrollment(row));
        }

        return enrollments;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find enrollments by semester: {}", e.what());
        throw std::runtime_error(std::string("Failed to find enrollments: ") + e.what());
    }
}

std::vector<Enrollment> PostgreSQLEnrollmentRepository::findByStatus(EnrollmentStatus status) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                WHERE status = $1
                ORDER BY student_id
            )",
            pqxx::params{
            statusToString(status)}
        );

        std::vector<Enrollment> enrollments;
        for (auto row : result) {
            enrollments.push_back(mapRowToEnrollment(row));
        }

        return enrollments;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find enrollments by status: {}", e.what());
        throw std::runtime_error(std::string("Failed to find enrollments: ") + e.what());
    }
}

bool PostgreSQLEnrollmentRepository::isEnrolled(int64_t studentId, int64_t courseId) const {
    auto result = findByStudentAndCourse(studentId, courseId);
    return result.has_value();
}

int64_t PostgreSQLEnrollmentRepository::countActiveEnrollments(int64_t courseId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT COUNT(*) as count FROM enrollments WHERE course_id = $1 AND status = 'ACTIVE'",
            pqxx::params{
            courseId}
        );

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count active enrollments: {}", e.what());
        throw std::runtime_error(std::string("Failed to count enrollments: ") + e.what());
    }
}

double PostgreSQLEnrollmentRepository::calculateStudentGPA(int64_t studentId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT 
                    COALESCE(ROUND(
                        SUM(gpa * credit_hours)::numeric / NULLIF(SUM(credit_hours), 0), 2
                    ), 0) as calculated_gpa
                FROM enrollments
                WHERE student_id = $1 AND grade_posted = true AND status IN ('COMPLETED', 'FAILED')
            )",
            pqxx::params{
            studentId}
        );

        if (result.empty()) {
            return 0.0;
        }

        return result[0]["calculated_gpa"].as<double>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to calculate student GPA: {}", e.what());
        throw std::runtime_error(std::string("Failed to calculate GPA: ") + e.what());
    }
}

void PostgreSQLEnrollmentRepository::updateStatus(int64_t enrollmentId, EnrollmentStatus status) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"UPDATE enrollments SET status = $2, updated_at = NOW() WHERE id = $1",
            pqxx::params{
            enrollmentId,
            statusToString(status)}
        );

        if (result.affected_rows() == 0) {
            throw std::runtime_error("Enrollment not found");
        }

        txn.commit();

        spdlog::info("Enrollment status updated: {} -> {}", enrollmentId, statusToString(status));

    } catch (const std::exception& e) {
        spdlog::error("Failed to update enrollment status: {}", e.what());
        throw std::runtime_error(std::string("Failed to update status: ") + e.what());
    }
}

void PostgreSQLEnrollmentRepository::postGrade(int64_t enrollmentId, double numericalGrade, const std::string& letterGrade) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        double gpa = letterToGPA(letterGrade);

        auto result = txn.exec(
R"(
                UPDATE enrollments
                SET grade = $2, letter_grade = $3, gpa = $4, grade_posted = true, updated_at = NOW()
                WHERE id = $1
            )",
            pqxx::params{
            enrollmentId,
            numericalGrade,
            letterGrade,
            gpa}
        );

        if (result.affected_rows() == 0) {
            throw std::runtime_error("Enrollment not found");
        }

        txn.commit();

        spdlog::info("Grade posted for enrollment {}: {} ({})", enrollmentId, numericalGrade, letterGrade);

    } catch (const std::exception& e) {
        spdlog::error("Failed to post grade: {}", e.what());
        throw std::runtime_error(std::string("Failed to post grade: ") + e.what());
    }
}

std::vector<Enrollment> PostgreSQLEnrollmentRepository::getUngradedEnrollments() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, student_id, course_id, status, semester, year, grade, grade_posted,
                       letter_grade, gpa, credit_hours, prerequisites_met
                FROM enrollments
                WHERE status IN ('COMPLETED', 'FAILED') AND grade_posted = false
                ORDER BY student_id
            )"
        );

        std::vector<Enrollment> enrollments;
        for (auto row : result) {
            enrollments.push_back(mapRowToEnrollment(row));
        }

        return enrollments;

    } catch (const std::exception& e) {
        spdlog::error("Failed to get ungraded enrollments: {}", e.what());
        throw std::runtime_error(std::string("Failed to get ungraded: ") + e.what());
    }
}

} // namespace sms::app::enrollments::repositories
