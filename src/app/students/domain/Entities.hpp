#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace sms::app::students::domain {

using json = nlohmann::json;
using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief Student academic standing enumeration
 */
enum class AcademicStanding {
    GOOD,        ///< GPA >= 2.0
    PROBATION,   ///< 1.0 <= GPA < 2.0
    WARNING,     ///< GPA < 1.0
    SUSPENDED    ///< Suspended for poor performance
};

/**
 * @brief Student status enumeration
 */
enum class StudentStatus {
    ACTIVE,      ///< Currently enrolled
    SUSPENDED,   ///< Suspended (disciplinary)
    GRADUATED,   ///< Successfully graduated
    INACTIVE,    ///< Not currently enrolled
    DELETED      ///< Soft deleted
};

/**
 * @brief Student entity
 * 
 * Represents a student record with personal, contact, and academic information.
 */
class Student {
public:
    Student() = default;

    /**
     * @brief Constructor with required fields
     * @param id Student ID
     * @param userId Associated user ID (from users table)
     * @param matricNumber Unique matric/registration number
     */
    Student(int64_t id, int64_t userId, const std::string& matricNumber)
        : m_id(id), m_userId(userId), m_matricNumber(matricNumber) {}

    // Getters
    [[nodiscard]] int64_t id() const { return m_id; }
    [[nodiscard]] int64_t userId() const { return m_userId; }
    [[nodiscard]] const std::string& matricNumber() const { return m_matricNumber; }
    [[nodiscard]] const std::string& firstName() const { return m_firstName; }
    [[nodiscard]] const std::string& lastName() const { return m_lastName; }
    [[nodiscard]] const std::string& email() const { return m_email; }
    [[nodiscard]] const std::string& phoneNumber() const { return m_phoneNumber; }
    [[nodiscard]] const std::optional<TimePoint>& dateOfBirth() const { return m_dateOfBirth; }
    [[nodiscard]] StudentStatus status() const { return m_status; }
    [[nodiscard]] AcademicStanding academicStanding() const { return m_academicStanding; }
    
    // Emergency Contact Info
    [[nodiscard]] const std::string& emergencyContactName() const { return m_emergencyContactName; }
    [[nodiscard]] const std::string& emergencyContactPhone() const { return m_emergencyContactPhone; }
    
    // Guardian Info
    [[nodiscard]] const std::string& guardianName() const { return m_guardianName; }
    [[nodiscard]] const std::string& guardianPhone() const { return m_guardianPhone; }
    [[nodiscard]] const std::string& guardianEmail() const { return m_guardianEmail; }
    
    // Timestamps
    [[nodiscard]] const TimePoint& createdAt() const { return m_createdAt; }
    [[nodiscard]] const TimePoint& updatedAt() const { return m_updatedAt; }

    // Setters
    void setId(int64_t id) { m_id = id; }
    void setUserId(int64_t userId) { m_userId = userId; }
    void setMatricNumber(const std::string& number) { m_matricNumber = number; }
    void setFirstName(const std::string& name) { m_firstName = name; }
    void setLastName(const std::string& name) { m_lastName = name; }
    void setEmail(const std::string& email) { m_email = email; }
    void setPhoneNumber(const std::string& phone) { m_phoneNumber = phone; }
    void setDateOfBirth(const TimePoint& dob) { m_dateOfBirth = dob; }
    void setStatus(StudentStatus status) { m_status = status; }
    void setAcademicStanding(AcademicStanding standing) { m_academicStanding = standing; }
    
    void setEmergencyContactName(const std::string& name) { m_emergencyContactName = name; }
    void setEmergencyContactPhone(const std::string& phone) { m_emergencyContactPhone = phone; }
    
    void setGuardianName(const std::string& name) { m_guardianName = name; }
    void setGuardianPhone(const std::string& phone) { m_guardianPhone = phone; }
    void setGuardianEmail(const std::string& email) { m_guardianEmail = email; }

    /**
     * @brief Get full name
     * @return First name + last name
     */
    [[nodiscard]] std::string getFullName() const {
        return m_firstName + " " + m_lastName;
    }

    /**
     * @brief Check if student is active
     * @return True if status is ACTIVE
     */
    [[nodiscard]] bool isActive() const {
        return m_status == StudentStatus::ACTIVE;
    }

    /**
     * @brief Check if student is good academic standing
     * @return True if standing is GOOD
     */
    [[nodiscard]] bool isInGoodStanding() const {
        return m_academicStanding == AcademicStanding::GOOD;
    }

    bool operator==(const Student& other) const {
        return m_id == other.m_id;
    }

private:
    int64_t m_id{0};
    int64_t m_userId{0};
    std::string m_matricNumber;
    std::string m_firstName;
    std::string m_lastName;
    std::string m_email;
    std::string m_phoneNumber;
    std::optional<TimePoint> m_dateOfBirth;
    StudentStatus m_status{StudentStatus::ACTIVE};
    AcademicStanding m_academicStanding{AcademicStanding::GOOD};
    
    // Emergency Contact
    std::string m_emergencyContactName;
    std::string m_emergencyContactPhone;
    
    // Guardian Information
    std::string m_guardianName;
    std::string m_guardianPhone;
    std::string m_guardianEmail;
    
    TimePoint m_createdAt;
    TimePoint m_updatedAt;
};

/**
 * @brief Convert status enum to string
 * @param status Student status
 * @return String representation
 */
inline std::string statusToString(StudentStatus status) {
    switch (status) {
        case StudentStatus::ACTIVE: return "ACTIVE";
        case StudentStatus::SUSPENDED: return "SUSPENDED";
        case StudentStatus::GRADUATED: return "GRADUATED";
        case StudentStatus::INACTIVE: return "INACTIVE";
        case StudentStatus::DELETED: return "DELETED";
    }
    return "ACTIVE";
}

/**
 * @brief Convert string to status enum
 * @param statusStr String representation
 * @return Status enum
 */
inline StudentStatus stringToStatus(const std::string& statusStr) {
    if (statusStr == "ACTIVE") return StudentStatus::ACTIVE;
    if (statusStr == "SUSPENDED") return StudentStatus::SUSPENDED;
    if (statusStr == "GRADUATED") return StudentStatus::GRADUATED;
    if (statusStr == "INACTIVE") return StudentStatus::INACTIVE;
    if (statusStr == "DELETED") return StudentStatus::DELETED;
    return StudentStatus::ACTIVE;
}

/**
 * @brief Convert standing enum to string
 * @param standing Academic standing
 * @return String representation
 */
inline std::string standingToString(AcademicStanding standing) {
    switch (standing) {
        case AcademicStanding::GOOD: return "GOOD";
        case AcademicStanding::PROBATION: return "PROBATION";
        case AcademicStanding::WARNING: return "WARNING";
        case AcademicStanding::SUSPENDED: return "SUSPENDED";
    }
    return "GOOD";
}

/**
 * @brief Convert string to standing enum
 * @param standingStr String representation
 * @return Standing enum
 */
inline AcademicStanding stringToStanding(const std::string& standingStr) {
    if (standingStr == "GOOD") return AcademicStanding::GOOD;
    if (standingStr == "PROBATION") return AcademicStanding::PROBATION;
    if (standingStr == "WARNING") return AcademicStanding::WARNING;
    if (standingStr == "SUSPENDED") return AcademicStanding::SUSPENDED;
    return AcademicStanding::GOOD;
}

} // namespace sms::app::students::domain
