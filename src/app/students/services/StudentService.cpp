#include "StudentService.hpp"
#include <spdlog/spdlog.h>
#include <regex>
#include <nlohmann/json.hpp>

namespace sms::app::students::services {

using json = nlohmann::json;

StudentService::StudentService(std::shared_ptr<IStudentRepository> studentRepo)
    : m_studentRepo(std::move(studentRepo)) {
    if (!m_studentRepo) {
        throw std::invalid_argument("Student repository cannot be null");
    }
}

void StudentService::validateEmail(const std::string& email) const {
    // Simple email validation
    std::regex emailPattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    if (!std::regex_match(email, emailPattern)) {
        throw std::runtime_error("Invalid email format: " + email);
    }
}

void StudentService::validateStudentInput(
    const std::string& firstName,
    const std::string& lastName,
    const std::string& email
) const {
    if (firstName.empty() || firstName.length() > 255) {
        throw std::runtime_error("First name must be between 1 and 255 characters");
    }

    if (lastName.empty() || lastName.length() > 255) {
        throw std::runtime_error("Last name must be between 1 and 255 characters");
    }

    validateEmail(email);
}

Student StudentService::registerStudent(
    int64_t userId,
    const std::string& matricNumber,
    const std::string& firstName,
    const std::string& lastName,
    const std::string& email,
    const std::string& phoneNumber
) {
    // Validate input
    if (matricNumber.empty()) {
        throw std::runtime_error("Matric number cannot be empty");
    }

    validateStudentInput(firstName, lastName, email);

    // Check if matric number already exists
    if (m_studentRepo->matricNumberExists(matricNumber)) {
        spdlog::warn("Registration attempt with existing matric number: {}", matricNumber);
        throw std::runtime_error("Matric number already registered");
    }

    // Create student
    Student newStudent(0, userId, matricNumber);
    newStudent.setFirstName(firstName);
    newStudent.setLastName(lastName);
    newStudent.setEmail(email);
    newStudent.setPhoneNumber(phoneNumber);
    newStudent.setStatus(StudentStatus::ACTIVE);
    newStudent.setAcademicStanding(AcademicStanding::GOOD);

    Student createdStudent = m_studentRepo->create(newStudent);
    spdlog::info("Student registered: {} (matric: {})", email, matricNumber);

    return createdStudent;
}

std::optional<Student> StudentService::getStudent(int64_t studentId) const {
    return m_studentRepo->read(studentId);
}

std::optional<Student> StudentService::getStudentByMatricNumber(const std::string& matricNumber) const {
    return m_studentRepo->findByMatricNumber(matricNumber);
}

std::optional<Student> StudentService::getStudentByUserId(int64_t userId) const {
    return m_studentRepo->findByUserId(userId);
}

Student StudentService::updateStudent(const Student& student) {
    auto existing = m_studentRepo->read(student.id());
    if (!existing) {
        throw std::runtime_error("Student not found");
    }

    validateStudentInput(student.firstName(), student.lastName(), student.email());

    Student updated = m_studentRepo->update(student);
    spdlog::info("Student updated: {} (id={})", student.email(), student.id());

    return updated;
}

void StudentService::suspendStudent(int64_t studentId, const std::string& reason) {
    auto student = m_studentRepo->read(studentId);
    if (!student) {
        throw std::runtime_error("Student not found");
    }

    student->setStatus(StudentStatus::SUSPENDED);
    m_studentRepo->update(*student);

    spdlog::warn("Student suspended: {} (id={}, reason: {})", 
        student->email(), studentId, reason);
}

void StudentService::reactivateStudent(int64_t studentId) {
    auto student = m_studentRepo->read(studentId);
    if (!student) {
        throw std::runtime_error("Student not found");
    }

    if (student->status() != StudentStatus::SUSPENDED) {
        throw std::runtime_error("Student is not suspended");
    }

    student->setStatus(StudentStatus::ACTIVE);
    m_studentRepo->update(*student);

    spdlog::info("Student reactivated: {} (id={})", student->email(), studentId);
}

void StudentService::graduateStudent(int64_t studentId) {
    auto student = m_studentRepo->read(studentId);
    if (!student) {
        throw std::runtime_error("Student not found");
    }

    student->setStatus(StudentStatus::GRADUATED);
    m_studentRepo->update(*student);

    spdlog::info("Student graduated: {} (id={})", student->email(), studentId);
}

bool StudentService::deleteStudent(int64_t studentId) {
    return m_studentRepo->delete_by_id(studentId);
}

std::vector<Student> StudentService::searchStudents(const std::string& searchTerm) const {
    if (searchTerm.empty()) {
        throw std::runtime_error("Search term cannot be empty");
    }

    return m_studentRepo->searchByName(searchTerm);
}

std::vector<Student> StudentService::getActiveStudents(int64_t offset, int64_t limit) const {
    std::vector<Student> all = m_studentRepo->readPage(offset, limit);
    std::vector<Student> active;

    for (const auto& student : all) {
        if (student.isActive()) {
            active.push_back(student);
        }
    }

    return active;
}

std::vector<Student> StudentService::getStudentsOnProbation() const {
    return m_studentRepo->getStudentsOnProbation();
}

void StudentService::updateAcademicStanding(int64_t studentId, AcademicStanding standing) {
    auto student = m_studentRepo->read(studentId);
    if (!student) {
        throw std::runtime_error("Student not found");
    }

    m_studentRepo->updateAcademicStanding(studentId, standing);
    spdlog::info("Student academic standing updated: {} (standing: {})", 
        studentId, standingToString(standing));
}

std::string StudentService::getStudentStatistics() const {
    try {
        json stats;
        stats["total"] = getTotalStudentCount();
        stats["active"] = getActiveStudentCount();
        stats["onProbation"] = getStudentsOnProbation().size();
        stats["suspended"] = m_studentRepo->findByStatus(StudentStatus::SUSPENDED).size();
        stats["graduated"] = m_studentRepo->findByStatus(StudentStatus::GRADUATED).size();

        return stats.dump();

    } catch (const std::exception& e) {
        spdlog::error("Failed to get student statistics: {}", e.what());
        throw std::runtime_error("Failed to get statistics");
    }
}

int64_t StudentService::getTotalStudentCount() const {
    return m_studentRepo->count();
}

int64_t StudentService::getActiveStudentCount() const {
    return m_studentRepo->countActiveStudents();
}

void StudentService::setEmergencyContact(
    int64_t studentId,
    const std::string& contactName,
    const std::string& contactPhone
) {
    auto student = m_studentRepo->read(studentId);
    if (!student) {
        throw std::runtime_error("Student not found");
    }

    if (contactName.empty()) {
        throw std::runtime_error("Contact name cannot be empty");
    }

    if (contactPhone.empty()) {
        throw std::runtime_error("Contact phone cannot be empty");
    }

    student->setEmergencyContactName(contactName);
    student->setEmergencyContactPhone(contactPhone);
    m_studentRepo->update(*student);

    spdlog::info("Emergency contact set for student: {} (id={})", contactName, studentId);
}

void StudentService::setGuardianInfo(
    int64_t studentId,
    const std::string& guardianName,
    const std::string& guardianPhone,
    const std::string& guardianEmail
) {
    auto student = m_studentRepo->read(studentId);
    if (!student) {
        throw std::runtime_error("Student not found");
    }

    if (guardianName.empty()) {
        throw std::runtime_error("Guardian name cannot be empty");
    }

    if (!guardianEmail.empty()) {
        validateEmail(guardianEmail);
    }

    student->setGuardianName(guardianName);
    student->setGuardianPhone(guardianPhone);
    student->setGuardianEmail(guardianEmail);
    m_studentRepo->update(*student);

    spdlog::info("Guardian info set for student: {} (id={})", guardianName, studentId);
}

} // namespace sms::app::students::services
