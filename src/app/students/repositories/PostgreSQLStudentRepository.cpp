#include "PostgreSQLStudentRepository.hpp"
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>

namespace sms::app::students::repositories {

PostgreSQLStudentRepository::PostgreSQLStudentRepository(std::shared_ptr<sms::database::ConnectionPool> pool)
    : m_pool(std::move(pool)) {
    if (!m_pool) {
        throw std::invalid_argument("Connection pool cannot be null");
    }
}

Student PostgreSQLStudentRepository::mapRowToStudent(const pqxx::row_ref& row) {
    Student student(
        row["id"].as<int64_t>(),
        row["user_id"].as<int64_t>(),
        row["matric_number"].as<std::string>()
    );

    student.setFirstName(row["first_name"].as<std::string>(""));
    student.setLastName(row["last_name"].as<std::string>(""));
    student.setEmail(row["email"].as<std::string>(""));
    student.setPhoneNumber(row["phone_number"].as<std::string>(""));

    if (!row["date_of_birth"].is_null()) {
        // Parse date if needed (for now skip time component)
    }

    student.setStatus(stringToStatus(row["status"].as<std::string>()));
    student.setAcademicStanding(stringToStanding(row["academic_standing"].as<std::string>("GOOD")));

    student.setEmergencyContactName(row["emergency_contact_name"].as<std::string>(""));
    student.setEmergencyContactPhone(row["emergency_contact_phone"].as<std::string>(""));

    student.setGuardianName(row["guardian_name"].as<std::string>(""));
    student.setGuardianPhone(row["guardian_phone"].as<std::string>(""));
    student.setGuardianEmail(row["guardian_email"].as<std::string>(""));

    return student;
}

Student PostgreSQLStudentRepository::create(const Student& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                INSERT INTO students
                (user_id, matric_number, phone_number,
                 emergency_contact_name, emergency_contact_phone, guardian_name,
                 guardian_phone, guardian_email, status, academic_standing, created_at, updated_at)
                VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, NOW(), NOW())
                RETURNING id, user_id, matric_number, phone_number,
                          emergency_contact_name, emergency_contact_phone, guardian_name,
                          guardian_phone, guardian_email, status, academic_standing
            )",
            pqxx::params{
            entity.userId(),
            entity.matricNumber(),
            entity.phoneNumber(),
            entity.emergencyContactName(),
            entity.emergencyContactPhone(),
            entity.guardianName(),
            entity.guardianPhone(),
            entity.guardianEmail(),
            statusToString(entity.status()),
            standingToString(entity.academicStanding())}
        );

        txn.commit();

        // Return the created student by reading it again (to get the joined user data)
        auto created = read(result[0]["id"].as<int64_t>());
        spdlog::info("Student created: {} (matric: {})", entity.email(), entity.matricNumber());
        return created.value_or(Student(0, 0, ""));

    } catch (const pqxx::unique_violation& e) {
        spdlog::error("Matric number already exists: {}", entity.matricNumber());
        throw std::runtime_error("Matric number already exists");
    } catch (const std::exception& e) {
        spdlog::error("Failed to create student: {}", e.what());
        throw std::runtime_error(std::string("Failed to create student: ") + e.what());
    }
}

std::optional<Student> PostgreSQLStudentRepository::read(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE s.id = $1 AND s.status != 'DELETED'
            )",
            pqxx::params{
            id}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToStudent(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to read student {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to read student: ") + e.what());
    }
}

Student PostgreSQLStudentRepository::update(const Student& entity) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        // Update user personal info in users table
        txn.exec(
            "UPDATE users SET first_name = $1, last_name = $2, email = $3, updated_at = NOW() WHERE id = $4",
            pqxx::params{entity.firstName(), entity.lastName(), entity.email(), entity.userId()}
        );

        // Update student specific info in students table
        auto result = txn.exec(
R"(
            UPDATE students
            SET user_id = $2, matric_number = $3, phone_number = $4, emergency_contact_name = $5,
                emergency_contact_phone = $6, guardian_name = $7, guardian_phone = $8,
                guardian_email = $9, status = $10, academic_standing = $11, updated_at = NOW()
            WHERE id = $1
            RETURNING id, user_id, matric_number, phone_number,
                      emergency_contact_name, emergency_contact_phone, guardian_name,
                      guardian_phone, guardian_email, status, academic_standing
        )",
            pqxx::params{
            entity.id(),
            entity.userId(),
            entity.matricNumber(),
            entity.phoneNumber(),
            entity.emergencyContactName(),
            entity.emergencyContactPhone(),
            entity.guardianName(),
            entity.guardianPhone(),
            entity.guardianEmail(),
            statusToString(entity.status()),
            standingToString(entity.academicStanding())}
        );

        if (result.empty()) {
            throw std::runtime_error("Student not found");
        }

        txn.commit();

        spdlog::info("Student updated: {} (id={})", entity.email(), entity.id());

        // Return the updated student by reading it again (to get the joined user data)
        auto updated = read(entity.id());
        return updated.value_or(entity);

    } catch (const std::exception& e) {
        spdlog::error("Failed to update student {}: {}", entity.id(), e.what());
        throw std::runtime_error(std::string("Failed to update student: ") + e.what());
    }
}

bool PostgreSQLStudentRepository::delete_by_id(int64_t id) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"UPDATE students SET status = 'DELETED', updated_at = NOW() WHERE id = $1",
            pqxx::params{
            id}
        );

        txn.commit();

        bool deleted = result.affected_rows() > 0;
        if (deleted) {
            spdlog::info("Student deleted: (id={})", id);
        }
        return deleted;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete student {}: {}", id, e.what());
        throw std::runtime_error(std::string("Failed to delete student: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::readAll() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE s.status != 'DELETED'
                ORDER BY u.last_name, u.first_name
            )"
        );

        std::vector<Student> students;
        for (auto row : result) {
            students.push_back(mapRowToStudent(row));
        }

        return students;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read all students: {}", e.what());
        throw std::runtime_error(std::string("Failed to read students: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::readPage(int64_t offset, int64_t limit) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE s.status != 'DELETED'
                ORDER BY u.last_name, u.first_name
                LIMIT $1 OFFSET $2
            )",
            pqxx::params{
            limit, offset}
        );

        std::vector<Student> students;
        for (auto row : result) {
            students.push_back(mapRowToStudent(row));
        }

        return students;

    } catch (const std::exception& e) {
        spdlog::error("Failed to read student page: {}", e.what());
        throw std::runtime_error(std::string("Failed to read students: ") + e.what());
    }
}

int64_t PostgreSQLStudentRepository::count() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT COUNT(*) as count FROM students WHERE status != 'DELETED'"
        );

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count students: {}", e.what());
        throw std::runtime_error(std::string("Failed to count students: ") + e.what());
    }
}

bool PostgreSQLStudentRepository::exists(int64_t id) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT 1 FROM students WHERE id = $1 AND status != 'DELETED'",
            pqxx::params{
            id}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check if student exists: {}", e.what());
        throw std::runtime_error(std::string("Failed to check student existence: ") + e.what());
    }
}

std::optional<Student> PostgreSQLStudentRepository::findByMatricNumber(const std::string& matricNumber) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE s.matric_number = $1 AND s.status != 'DELETED'
            )",
            pqxx::params{
            matricNumber}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToStudent(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to find student by matric number: {}", e.what());
        throw std::runtime_error(std::string("Failed to find student: ") + e.what());
    }
}

std::optional<Student> PostgreSQLStudentRepository::findByUserId(int64_t userId) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE s.user_id = $1 AND s.status != 'DELETED'
            )",
            pqxx::params{
            userId}
        );

        if (result.empty()) {
            return std::nullopt;
        }

        return mapRowToStudent(result[0]);

    } catch (const std::exception& e) {
        spdlog::error("Failed to find student by user ID: {}", e.what());
        throw std::runtime_error(std::string("Failed to find student: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::findByStatus(StudentStatus status) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE s.status = $1
                ORDER BY u.last_name, u.first_name
            )",
            pqxx::params{
            statusToString(status)}
        );

        std::vector<Student> students;
        for (auto row : result) {
            students.push_back(mapRowToStudent(row));
        }

        return students;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find students by status: {}", e.what());
        throw std::runtime_error(std::string("Failed to find students: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::findByAcademicStanding(AcademicStanding standing) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE s.academic_standing = $1 AND s.status != 'DELETED'
                ORDER BY u.last_name, u.first_name
            )",
            pqxx::params{
            standingToString(standing)}
        );

        std::vector<Student> students;
        for (auto row : result) {
            students.push_back(mapRowToStudent(row));
        }

        return students;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find students by academic standing: {}", e.what());
        throw std::runtime_error(std::string("Failed to find students: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::searchByName(const std::string& name) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        std::string searchTerm = "%" + name + "%";
        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE (u.first_name ILIKE $1 OR u.last_name ILIKE $1 OR u.email ILIKE $1)
                AND s.status != 'DELETED'
                ORDER BY u.last_name, u.first_name
                LIMIT 50
            )",
            pqxx::params{
            searchTerm}
        );

        std::vector<Student> students;
        for (auto row : result) {
            students.push_back(mapRowToStudent(row));
        }

        return students;

    } catch (const std::exception& e) {
        spdlog::error("Failed to search students by name: {}", e.what());
        throw std::runtime_error(std::string("Failed to search students: ") + e.what());
    }
}

bool PostgreSQLStudentRepository::matricNumberExists(const std::string& matricNumber) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"SELECT 1 FROM students WHERE matric_number = $1 AND status != 'DELETED'",
            pqxx::params{
            matricNumber}
        );

        return !result.empty();

    } catch (const std::exception& e) {
        spdlog::error("Failed to check matric number existence: {}", e.what());
        throw std::runtime_error(std::string("Failed to check matric number: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::getStudentsOnProbation() const {
    return findByAcademicStanding(AcademicStanding::PROBATION);
}

int64_t PostgreSQLStudentRepository::countActiveStudents() const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT COUNT(*) as count FROM students WHERE status = 'ACTIVE'"
        );

        return result[0]["count"].as<int64_t>();

    } catch (const std::exception& e) {
        spdlog::error("Failed to count active students: {}", e.what());
        throw std::runtime_error(std::string("Failed to count students: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::getGraduatedStudents(int64_t offset, int64_t limit) const {
    return readPage(offset, limit); // Placeholder - would filter by GRADUATED status
}

void PostgreSQLStudentRepository::updateAcademicStanding(int64_t studentId, AcademicStanding standing) {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
"UPDATE students SET academic_standing = $2, updated_at = NOW() WHERE id = $1",
            pqxx::params{
            studentId,
            standingToString(standing)}
        );

        if (result.affected_rows() == 0) {
            throw std::runtime_error("Student not found");
        }

        txn.commit();

        spdlog::info("Student academic standing updated: {} -> {}",
            studentId, standingToString(standing));

    } catch (const std::exception& e) {
        spdlog::error("Failed to update academic standing: {}", e.what());
        throw std::runtime_error(std::string("Failed to update standing: ") + e.what());
    }
}

std::vector<Student> PostgreSQLStudentRepository::findByNameParts(
    const std::string& firstName,
    const std::string& lastName
) const {
    try {
        auto conn = m_pool->acquireConnection();
        pqxx::work txn(*conn);

        std::string firstNameTerm = "%" + firstName + "%";
        std::string lastNameTerm = "%" + lastName + "%";

        auto result = txn.exec(
R"(
                SELECT s.id, s.user_id, s.matric_number, u.first_name, u.last_name, u.email, s.phone_number,
                       s.emergency_contact_name, s.emergency_contact_phone, s.guardian_name,
                       s.guardian_phone, s.guardian_email, s.status, s.academic_standing
                FROM students s
                JOIN users u ON s.user_id = u.id
                WHERE u.first_name ILIKE $1 AND u.last_name ILIKE $2
                AND s.status != 'DELETED'
                ORDER BY u.last_name, u.first_name
            )",
            pqxx::params{
            firstNameTerm,
            lastNameTerm}
        );

        std::vector<Student> students;
        for (auto row : result) {
            students.push_back(mapRowToStudent(row));
        }

        return students;

    } catch (const std::exception& e) {
        spdlog::error("Failed to find students by name parts: {}", e.what());
        throw std::runtime_error(std::string("Failed to find students: ") + e.what());
    }
}

} // namespace sms::app::students::repositories
