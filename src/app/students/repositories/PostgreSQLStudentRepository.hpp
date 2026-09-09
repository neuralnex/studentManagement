#pragma once

#include "IStudentRepository.hpp"
#include "database/ConnectionPool.hpp"
#include <memory>

namespace sms::app::students::repositories {

/**
 * @brief PostgreSQL implementation of IStudentRepository
 */
class PostgreSQLStudentRepository : public IStudentRepository {
public:
    explicit PostgreSQLStudentRepository(std::shared_ptr<sms::database::ConnectionPool> pool);

    Student create(const Student& entity) override;
    std::optional<Student> read(int64_t id) const override;
    Student update(const Student& entity) override;
    bool delete_by_id(int64_t id) override;
    std::vector<Student> readAll() const override;
    std::vector<Student> readPage(int64_t offset, int64_t limit) const override;
    int64_t count() const override;
    bool exists(int64_t id) const override;

    std::optional<Student> findByMatricNumber(const std::string& matricNumber) const override;
    std::optional<Student> findByUserId(int64_t userId) const override;
    std::vector<Student> findByStatus(StudentStatus status) const override;
    std::vector<Student> findByAcademicStanding(AcademicStanding standing) const override;
    std::vector<Student> searchByName(const std::string& name) const override;
    bool matricNumberExists(const std::string& matricNumber) const override;
    std::vector<Student> getStudentsOnProbation() const override;
    int64_t countActiveStudents() const override;
    std::vector<Student> getGraduatedStudents(int64_t offset, int64_t limit) const override;
    void updateAcademicStanding(int64_t studentId, AcademicStanding standing) override;
    std::vector<Student> findByNameParts(
        const std::string& firstName,
        const std::string& lastName
    ) const override;

private:
    std::shared_ptr<sms::database::ConnectionPool> m_pool;

    static Student mapRowToStudent(const pqxx::row_ref& row);
};

} // namespace sms::app::students::repositories
