#include "PostgreSQLCourseRepository.hpp"
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>

namespace sms::app::courses::repositories {

// PostgreSQLCourseRepository Implementation

PostgreSQLCourseRepository::PostgreSQLCourseRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(std::move(pool)) {
    if (!m_pool) {
        throw std::invalid_argument("Connection pool cannot be null");
    }
}

Course PostgreSQLCourseRepository::mapRowToCourse(const pqxx::row_ref& row) {
    Course course(
        row["id"].as<int64_t>(),
        row["code"].as<std::string>(),
        row["title"].as<std::string>(),
        row["credit_units"].as<int>()
    );

    course.setDescription(row["description"].as<std::string>(""));
    course.setCapacity(row["capacity"].as<int>());
    course.setEnrolledCount(row["enrolled_count"].as<int>(0));
    course.setDepartmentId(row["department_id"].as<int64_t>());
    course.setLecturerId(row["lecturer_id"].as<int64_t>());
    course.setStatus(stringToStatus(row["status"].as<std::string>()));
    course.setSemester(stringToSemester(row["semester"].as<std::string>()));
    course.setYear(row["year"].as<int>());
    course.setLocation(row["location"].as<std::string>(""));
    course.setSchedule(row["schedule"].as<std::string>(""));

    return course;
}

Course PostgreSQLCourseRepository::create(const Course& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                INSERT INTO courses 
                (code, title, description, credit_units, capacity, enrolled_count,
                 department_id, lecturer_id, status, semester, year, location, schedule, created_at, updated_at)
                VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, NOW(), NOW())
                RETURNING id, code, title, description, credit_units, capacity, enrolled_count,
                          department_id, lecturer_id, status, semester, year, location, schedule
            )",
            pqxx::params{
            entity.code(),
            entity.title(),
            entity.description(),
            entity.creditUnits(),
            entity.capacity(),
            entity.enrolledCount(),
            entity.departmentId(),
            entity.lecturerId(),
            statusToString(entity.status()),
            semesterToString(entity.semester()),
            entity.year(),
            entity.location(),
            entity.schedule()}
        );

        txn.commit();

        Course created = mapRowToCourse(result[0]);
        spdlog::info("Course created: {} ({})", entity.title(), entity.code());
        return created;

    } catch (const pqxx::unique_violation& e) {
        spdlog::error("Course code already exists: {}", entity.code());
        throw std::runtime_error("Course code already exists");
    } catch (const std::exception& e) {
        spdlog::error("Failed to create course: {}", e.what());
        throw std::runtime_error(std::string("Failed to create course: ") + e.what());
    }
}

std::optional<Course> PostgreSQLCourseRepository::read(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE id = $1
            )",
            pqxx::params{
            id}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToCourse(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to read course {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to read course: ") + e.what());
    }
}

Course PostgreSQLCourseRepository::update(const Course& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                UPDATE courses
                SET code = $2, title = $3, description = $4, credit_units = $5,
                    capacity = $6, enrolled_count = $7, department_id = $8, lecturer_id = $9,
                    status = $10, semester = $11, year = $12, location = $13, schedule = $14, updated_at = NOW()
                WHERE id = $1
                RETURNING id, code, title, description, credit_units, capacity, enrolled_count,
                          department_id, lecturer_id, status, semester, year, location, schedule
            )",
            pqxx::params{
            entity.id(),
            entity.code(),
            entity.title(),
            entity.description(),
            entity.creditUnits(),
            entity.capacity(),
            entity.enrolledCount(),
            entity.departmentId(),
            entity.lecturerId(),
            statusToString(entity.status()),
            semesterToString(entity.semester()),
            entity.year(),
            entity.location(),
            entity.schedule()}
        );

        if (result.empty()) {
            throw std::runtime_error("Course not found");
        }

        txn.commit();

        spdlog::info("Course updated: {} (id={})", entity.title(), entity.id());
        return mapRowToCourse(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to update course {}: {}", entity.id(), e.what());
        throw std::runtime_error(std::string("Failed to update course: ") + e.what());
    }
}

bool PostgreSQLCourseRepository::delete_by_id(int64_t id) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"DELETE FROM courses WHERE id = $1",
            pqxx::params{
            id}
        );

        txn.commit();

        bool deleted = result.affected_rows() > 0;
        if (deleted) {
            spdlog::info("Course deleted: (id={})", id);
        }
        return deleted;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete course {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to delete course: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::readAll() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                ORDER BY code
            )"
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read all courses: {}", e.what());
        throw std::runtime_error(std::string("Failed to read courses: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::readPage(int64_t offset, int64_t limit) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                ORDER BY code
                LIMIT $1 OFFSET $2
            )",
            pqxx::params{
            limit, offset}
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read course page: {}", e.what());
        throw std::runtime_error(std::string("Failed to read courses: ") + e.what());
    }
}

int64_t PostgreSQLCourseRepository::count() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec("SELECT COUNT(*) as count FROM courses");

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count courses: {}", e.what());
        throw std::runtime_error(std::string("Failed to count courses: ") + e.what());
    }
}

bool PostgreSQLCourseRepository::exists(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT 1 FROM courses WHERE id = $1",
            pqxx::params{
            id}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check if course exists: {}", e.what());
        throw std::runtime_error(std::string("Failed to check course existence: ") + e.what());
    }
}

std::optional<Course> PostgreSQLCourseRepository::findByCode(const std::string& code) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE code = $1
            )",
            pqxx::params{
            code}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToCourse(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to find course by code: {}", e.what());
        throw std::runtime_error(std::string("Failed to find course: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::findByDepartment(int64_t departmentId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE department_id = $1
                ORDER BY code
            )",
            pqxx::params{
            departmentId}
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find courses by department: {}", e.what());
        throw std::runtime_error(std::string("Failed to find courses: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::findByLecturer(int64_t lecturerId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE lecturer_id = $1
                ORDER BY code
            )",
            pqxx::params{
            lecturerId}
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find courses by lecturer: {}", e.what());
        throw std::runtime_error(std::string("Failed to find courses: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::findBySemester(Semester semester, int year) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE semester = $1 AND year = $2
                ORDER BY code
            )",
            pqxx::params{
            semesterToString(semester),
            year}
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find courses by semester: {}", e.what());
        throw std::runtime_error(std::string("Failed to find courses: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::findActive() const {
    return findByStatus(CourseStatus::ACTIVE);
}

std::vector<Course> PostgreSQLCourseRepository::findWithAvailableSlots() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE enrolled_count < capacity AND status = 'ACTIVE'
                ORDER BY code
            )"
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find courses with available slots: {}", e.what());
        throw std::runtime_error(std::string("Failed to find courses: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::search(const std::string& searchTerm) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        std::string term = "%" + searchTerm + "%";
        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE code ILIKE $1 OR title ILIKE $1 OR description ILIKE $1
                ORDER BY code
                LIMIT 50
            )",
            pqxx::params{
            term}
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to search courses: {}", e.what());
        throw std::runtime_error(std::string("Failed to search courses: ") + e.what());
    }
}

std::vector<Course> PostgreSQLCourseRepository::findByStatus(CourseStatus status) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT id, code, title, description, credit_units, capacity, enrolled_count,
                       department_id, lecturer_id, status, semester, year, location, schedule
                FROM courses
                WHERE status = $1
                ORDER BY code
            )",
            pqxx::params{
            statusToString(status)}
        );

        std::vector<Course> courses;
        for (auto row : result) {
            courses.push_back(mapRowToCourse(row));
        }

        return courses;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find courses by status: {}", e.what());
        throw std::runtime_error(std::string("Failed to find courses: ") + e.what());
    }
}

void PostgreSQLCourseRepository::updateEnrollmentCount(int64_t courseId, int newCount) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"UPDATE courses SET enrolled_count = $2, updated_at = NOW() WHERE id = $1",
            pqxx::params{
            courseId,
            newCount}
        );

        if (result.affected_rows() == 0) {
            throw std::runtime_error("Course not found");
        }

        txn.commit();

        spdlog::debug("Course enrollment count updated: {} -> {}", courseId, newCount);

    } catch (const std::exception& e) {
        spdlog::error("Failed to update enrollment count: {}", e.what());
        throw std::runtime_error(std::string("Failed to update enrollment: ") + e.what());
    }
}

bool PostgreSQLCourseRepository::codeExists(const std::string& code) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT 1 FROM courses WHERE code = $1",
            pqxx::params{
            code}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check course code existence: {}", e.what());
        throw std::runtime_error(std::string("Failed to check code: ") + e.what());
    }
}

// PostgreSQLCoursePrerequisiteRepository Implementation

PostgreSQLCoursePrerequisiteRepository::PostgreSQLCoursePrerequisiteRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(std::move(pool)) {
    if (!m_pool) {
        throw std::invalid_argument("Connection pool cannot be null");
    }
}

CoursePrerequisite PostgreSQLCoursePrerequisiteRepository::mapRowToPrerequisite(const pqxx::row_ref& row) {
    return CoursePrerequisite(
        row["course_id"].as<int64_t>(),
        row["prerequisite_id"].as<int64_t>(),
        row["min_grade"].as<std::string>("")
    );
}

CoursePrerequisite PostgreSQLCoursePrerequisiteRepository::create(const CoursePrerequisite& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        txn.exec(
R"(
                INSERT INTO course_prerequisites (course_id, prerequisite_id, min_grade, created_at)
                VALUES ($1, $2, $3, NOW())
            )",
            pqxx::params{
            entity.courseId(),
            entity.prerequisiteCourseId(),
            entity.minGrade()}
        );

        txn.commit();

        spdlog::info("Prerequisite added to course: {} -> {}", 
            entity.courseId(), entity.prerequisiteCourseId());
        return entity;

    } catch (const std::exception& e) {
        spdlog::error("Failed to create prerequisite: {}", e.what());
        throw std::runtime_error(std::string("Failed to create prerequisite: ") + e.what());
    }
}

std::optional<CoursePrerequisite> PostgreSQLCoursePrerequisiteRepository::read(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT course_id, prerequisite_id, min_grade FROM course_prerequisites WHERE id = $1",
            pqxx::params{
            id}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToPrerequisite(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to read prerequisite: {}", e.what());
        throw std::runtime_error(std::string("Failed to read prerequisite: ") + e.what());
    }
}

CoursePrerequisite PostgreSQLCoursePrerequisiteRepository::update(const CoursePrerequisite& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        txn.exec(
"UPDATE course_prerequisites SET min_grade = $2 WHERE course_id = $1 AND prerequisite_id = $3",
            pqxx::params{
            entity.courseId(),
            entity.minGrade(),
            entity.prerequisiteCourseId()}
        );

        txn.commit();

        return entity;

    } catch (const std::exception& e) {
        spdlog::error("Failed to update prerequisite: {}", e.what());
        throw std::runtime_error(std::string("Failed to update prerequisite: ") + e.what());
    }
}

bool PostgreSQLCoursePrerequisiteRepository::delete_by_id(int64_t id) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"DELETE FROM course_prerequisites WHERE id = $1",
            pqxx::params{
            id}
        );

        txn.commit();

        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete prerequisite: {}", e.what());
        throw std::runtime_error(std::string("Failed to delete prerequisite: ") + e.what());
    }
}

std::vector<CoursePrerequisite> PostgreSQLCoursePrerequisiteRepository::readAll() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT course_id, prerequisite_id, min_grade FROM course_prerequisites"
        );

        std::vector<CoursePrerequisite> prerequisites;
        for (auto row : result) {
            prerequisites.push_back(mapRowToPrerequisite(row));
        }

        return prerequisites;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read all prerequisites: {}", e.what());
        throw std::runtime_error(std::string("Failed to read prerequisites: ") + e.what());
    }
}

std::vector<CoursePrerequisite> PostgreSQLCoursePrerequisiteRepository::readPage(int64_t offset, int64_t limit) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT course_id, prerequisite_id, min_grade FROM course_prerequisites LIMIT $1 OFFSET $2",
            pqxx::params{
            limit, offset}
        );

        std::vector<CoursePrerequisite> prerequisites;
        for (auto row : result) {
            prerequisites.push_back(mapRowToPrerequisite(row));
        }

        return prerequisites;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read prerequisite page: {}", e.what());
        throw std::runtime_error(std::string("Failed to read prerequisites: ") + e.what());
    }
}

int64_t PostgreSQLCoursePrerequisiteRepository::count() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec("SELECT COUNT(*) as count FROM course_prerequisites");

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count prerequisites: {}", e.what());
        throw std::runtime_error(std::string("Failed to count prerequisites: ") + e.what());
    }
}

bool PostgreSQLCoursePrerequisiteRepository::exists(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT 1 FROM course_prerequisites WHERE id = $1",
            pqxx::params{
            id}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check prerequisite existence: {}", e.what());
        throw std::runtime_error(std::string("Failed to check existence: ") + e.what());
    }
}

std::vector<CoursePrerequisite> PostgreSQLCoursePrerequisiteRepository::getPrerequisites(int64_t courseId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT course_id, prerequisite_id, min_grade FROM course_prerequisites WHERE course_id = $1",
            pqxx::params{
            courseId}
        );

        std::vector<CoursePrerequisite> prerequisites;
        for (auto row : result) {
            prerequisites.push_back(mapRowToPrerequisite(row));
        }

        return prerequisites;

    } catch (const std::exception& e) {
        spdlog::error("Failed to get prerequisites for course: {}", e.what());
        throw std::runtime_error(std::string("Failed to get prerequisites: ") + e.what());
    }
}

CoursePrerequisite PostgreSQLCoursePrerequisiteRepository::addPrerequisite(
    int64_t courseId,
    int64_t prerequisiteId,
    const std::string& minGrade
) {
    CoursePrerequisite prereq(courseId, prerequisiteId, minGrade);
    return create(prereq);
}

bool PostgreSQLCoursePrerequisiteRepository::removePrerequisite(int64_t courseId, int64_t prerequisiteId) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"DELETE FROM course_prerequisites WHERE course_id = $1 AND prerequisite_id = $2",
            pqxx::params{
            courseId,
            prerequisiteId}
        );

        txn.commit();

        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        spdlog::error("Failed to remove prerequisite: {}", e.what());
        throw std::runtime_error(std::string("Failed to remove prerequisite: ") + e.what());
    }
}

bool PostgreSQLCoursePrerequisiteRepository::isPrerequisiteFor(int64_t courseId, int64_t targetCourseId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT 1 FROM course_prerequisites WHERE prerequisite_id = $1 AND course_id = $2",
            pqxx::params{
            courseId,
            targetCourseId}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check prerequisite relationship: {}", e.what());
        throw std::runtime_error(std::string("Failed to check relationship: ") + e.what());
    }
}

std::vector<CoursePrerequisite> PostgreSQLCoursePrerequisiteRepository::getRequiredByCoursees(int64_t courseId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT course_id, prerequisite_id, min_grade FROM course_prerequisites WHERE prerequisite_id = $1",
            pqxx::params{
            courseId}
        );

        std::vector<CoursePrerequisite> prerequisites;
        for (auto row : result) {
            prerequisites.push_back(mapRowToPrerequisite(row));
        }

        return prerequisites;

    } catch (const std::exception& e) {
        spdlog::error("Failed to get courses requiring prerequisite: {}", e.what());
        throw std::runtime_error(std::string("Failed to get courses: ") + e.what());
    }
}

} // namespace sms::app::courses::repositories
