#pragma once

#include "ICourseRepository.hpp"
#include "database/ConnectionPool.hpp"
#include <memory>

namespace sms::app::courses::repositories {

/**
 * @brief PostgreSQL implementation of ICourseRepository
 */
class PostgreSQLCourseRepository : public ICourseRepository {
public:
    explicit PostgreSQLCourseRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    Course create(const Course& entity) override;
    std::optional<Course> read(int64_t id) const override;
    Course update(const Course& entity) override;
    bool delete_by_id(int64_t id) override;
    std::vector<Course> readAll() const override;
    std::vector<Course> readPage(int64_t offset, int64_t limit) const override;
    int64_t count() const override;
    bool exists(int64_t id) const override;

    std::optional<Course> findByCode(const std::string& code) const override;
    std::vector<Course> findByDepartment(int64_t departmentId) const override;
    std::vector<Course> findByLecturer(int64_t lecturerId) const override;
    std::vector<Course> findBySemester(Semester semester, int year) const override;
    std::vector<Course> findActive() const override;
    std::vector<Course> findWithAvailableSlots() const override;
    std::vector<Course> search(const std::string& searchTerm) const override;
    std::vector<Course> findByStatus(CourseStatus status) const override;
    void updateEnrollmentCount(int64_t courseId, int newCount) override;
    bool codeExists(const std::string& code) const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;

    static Course mapRowToCourse(const pqxx::row_ref& row);
};

/**
 * @brief PostgreSQL implementation of ICoursePrerequisiteRepository
 */
class PostgreSQLCoursePrerequisiteRepository : public ICoursePrerequisiteRepository {
public:
    explicit PostgreSQLCoursePrerequisiteRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    CoursePrerequisite create(const CoursePrerequisite& entity) override;
    std::optional<CoursePrerequisite> read(int64_t id) const override;
    CoursePrerequisite update(const CoursePrerequisite& entity) override;
    bool delete_by_id(int64_t id) override;
    std::vector<CoursePrerequisite> readAll() const override;
    std::vector<CoursePrerequisite> readPage(int64_t offset, int64_t limit) const override;
    int64_t count() const override;
    bool exists(int64_t id) const override;

    std::vector<CoursePrerequisite> getPrerequisites(int64_t courseId) const override;
    CoursePrerequisite addPrerequisite(
        int64_t courseId,
        int64_t prerequisiteId,
        const std::string& minGrade = ""
    ) override;
    bool removePrerequisite(int64_t courseId, int64_t prerequisiteId) override;
    bool isPrerequisiteFor(int64_t courseId, int64_t targetCourseId) const override;
    std::vector<CoursePrerequisite> getRequiredByCoursees(int64_t courseId) const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;

    static CoursePrerequisite mapRowToPrerequisite(const pqxx::row_ref& row);
};

} // namespace sms::app::courses::repositories
