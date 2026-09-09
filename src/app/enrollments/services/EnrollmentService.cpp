#include "EnrollmentService.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cmath>

namespace sms::app::enrollments::services {

using json = nlohmann::json;

EnrollmentService::EnrollmentService(
    std::shared_ptr<IEnrollmentRepository> enrollmentRepo,
    std::shared_ptr<sms::app::courses::repositories::ICourseRepository> courseRepo,
    std::shared_ptr<sms::app::students::repositories::IStudentRepository> studentRepo
)
    : m_enrollmentRepo(std::move(enrollmentRepo)),
      m_courseRepo(std::move(courseRepo)),
      m_studentRepo(std::move(studentRepo)) {
    if (!m_enrollmentRepo) {
        throw std::invalid_argument("Enrollment repository cannot be null");
    }
}

std::string EnrollmentService::gradeToLetter(double numericGrade) {
    if (numericGrade >= 90.0) return "A";
    if (numericGrade >= 80.0) return "B";
    if (numericGrade >= 70.0) return "C";
    if (numericGrade >= 60.0) return "D";
    return "F";
}

Enrollment EnrollmentService::enrollStudent(
    int64_t studentId,
    int64_t courseId,
    const std::string& semester,
    int year
) {
    try {
        // Check if already enrolled
        if (m_enrollmentRepo->isEnrolled(studentId, courseId)) {
            throw std::runtime_error("Student is already enrolled in this course");
        }

        // Validate course exists and has capacity (if course repo available)
        if (m_courseRepo) {
            auto course = m_courseRepo->read(courseId);
            if (!course) {
                throw std::runtime_error("Course not found");
            }

            if (!course->hasAvailableSlots()) {
                throw std::runtime_error("Course is at capacity");
            }
        }

        // Validate student exists (if student repo available)
        if (m_studentRepo) {
            auto student = m_studentRepo->read(studentId);
            if (!student) {
                throw std::runtime_error("Student not found");
            }

            if (!student->isActive()) {
                throw std::runtime_error("Student is not in active status");
            }
        }

        // Create enrollment
        Enrollment enrollment(0, studentId, courseId);
        enrollment.setSemester(semester);
        enrollment.setYear(year);
        enrollment.setStatus(EnrollmentStatus::PENDING);
        enrollment.setPrerequisitesMet(checkPrerequisites(studentId, courseId));

        Enrollment created = m_enrollmentRepo->create(enrollment);

        spdlog::info("Student {} enrolled in course {}", studentId, courseId);
        return created;

    } catch (const std::exception& e) {
        spdlog::error("Failed to enroll student: {}", e.what());
        throw std::runtime_error(std::string("Enrollment failed: ") + e.what());
    }
}

std::vector<Enrollment> EnrollmentService::getStudentEnrollments(int64_t studentId) const {
    return m_enrollmentRepo->findByStudent(studentId);
}

std::vector<Enrollment> EnrollmentService::getCourseEnrollments(int64_t courseId) const {
    return m_enrollmentRepo->findByCourse(courseId);
}

void EnrollmentService::dropCourse(int64_t enrollmentId) {
    try {
        auto enrollment = m_enrollmentRepo->read(enrollmentId);
        if (!enrollment) {
            throw std::runtime_error("Enrollment not found");
        }

        if (enrollment->status() != EnrollmentStatus::PENDING &&
            enrollment->status() != EnrollmentStatus::APPROVED) {
            throw std::runtime_error("Can only drop pending or approved enrollments");
        }

        m_enrollmentRepo->updateStatus(enrollmentId, EnrollmentStatus::DROPPED);

        spdlog::info("Course dropped: enrollment {}", enrollmentId);

    } catch (const std::exception& e) {
        spdlog::error("Failed to drop course: {}", e.what());
        throw std::runtime_error(std::string("Drop failed: ") + e.what());
    }
}

void EnrollmentService::withdrawFromCourse(int64_t enrollmentId) {
    try {
        auto enrollment = m_enrollmentRepo->read(enrollmentId);
        if (!enrollment) {
            throw std::runtime_error("Enrollment not found");
        }

        if (enrollment->status() != EnrollmentStatus::ACTIVE) {
            throw std::runtime_error("Can only withdraw from active enrollments");
        }

        m_enrollmentRepo->updateStatus(enrollmentId, EnrollmentStatus::WITHDRAWN);

        spdlog::info("Course withdrawn: enrollment {}", enrollmentId);

    } catch (const std::exception& e) {
        spdlog::error("Failed to withdraw from course: {}", e.what());
        throw std::runtime_error(std::string("Withdrawal failed: ") + e.what());
    }
}

void EnrollmentService::approveEnrollment(int64_t enrollmentId) {
    try {
        auto enrollment = m_enrollmentRepo->read(enrollmentId);
        if (!enrollment) {
            throw std::runtime_error("Enrollment not found");
        }

        if (enrollment->status() != EnrollmentStatus::PENDING) {
            throw std::runtime_error("Can only approve pending enrollments");
        }

        m_enrollmentRepo->updateStatus(enrollmentId, EnrollmentStatus::APPROVED);

        spdlog::info("Enrollment approved: {}", enrollmentId);

    } catch (const std::exception& e) {
        spdlog::error("Failed to approve enrollment: {}", e.what());
        throw std::runtime_error(std::string("Approval failed: ") + e.what());
    }
}

void EnrollmentService::activateEnrollment(int64_t enrollmentId) {
    try {
        auto enrollment = m_enrollmentRepo->read(enrollmentId);
        if (!enrollment) {
            throw std::runtime_error("Enrollment not found");
        }

        if (enrollment->status() != EnrollmentStatus::APPROVED) {
            throw std::runtime_error("Can only activate approved enrollments");
        }

        m_enrollmentRepo->updateStatus(enrollmentId, EnrollmentStatus::ACTIVE);

        spdlog::info("Enrollment activated: {}", enrollmentId);

    } catch (const std::exception& e) {
        spdlog::error("Failed to activate enrollment: {}", e.what());
        throw std::runtime_error(std::string("Activation failed: ") + e.what());
    }
}

void EnrollmentService::completeEnrollment(int64_t enrollmentId) {
    try {
        auto enrollment = m_enrollmentRepo->read(enrollmentId);
        if (!enrollment) {
            throw std::runtime_error("Enrollment not found");
        }

        if (enrollment->status() != EnrollmentStatus::ACTIVE) {
            throw std::runtime_error("Can only complete active enrollments");
        }

        m_enrollmentRepo->updateStatus(enrollmentId, EnrollmentStatus::COMPLETED);

        spdlog::info("Enrollment completed: {}", enrollmentId);

    } catch (const std::exception& e) {
        spdlog::error("Failed to complete enrollment: {}", e.what());
        throw std::runtime_error(std::string("Completion failed: ") + e.what());
    }
}

void EnrollmentService::postGrade(int64_t enrollmentId, double numericalGrade) {
    try {
        if (numericalGrade < 0.0 || numericalGrade > 100.0) {
            throw std::runtime_error("Grade must be between 0 and 100");
        }

        auto enrollment = m_enrollmentRepo->read(enrollmentId);
        if (!enrollment) {
            throw std::runtime_error("Enrollment not found");
        }

        if (!enrollment->isCompleted()) {
            throw std::runtime_error("Can only grade completed enrollments");
        }

        std::string letterGrade = gradeToLetter(numericalGrade);
        m_enrollmentRepo->postGrade(enrollmentId, numericalGrade, letterGrade);

        spdlog::info("Grade posted: enrollment {} -> {} ({})", enrollmentId, numericalGrade, letterGrade);

    } catch (const std::exception& e) {
        spdlog::error("Failed to post grade: {}", e.what());
        throw std::runtime_error(std::string("Grade posting failed: ") + e.what());
    }
}

double EnrollmentService::getStudentGPA(int64_t studentId) const {
    return m_enrollmentRepo->calculateStudentGPA(studentId);
}

std::string EnrollmentService::generateTranscript(int64_t studentId) const {
    try {
        json transcript;
        transcript["studentId"] = studentId;
        transcript["generatedDate"] = "2026-08-01";

        auto enrollments = m_enrollmentRepo->findByStudent(studentId);

        json courses = json::array();
        int gradeCount = 0;

        for (const auto& enrollment : enrollments) {
            if (enrollment.gradePosted()) {
                json course;
                course["courseId"] = enrollment.courseId();
                course["semester"] = enrollment.semester();
                course["grade"] = enrollment.letterGrade();
                course["numericalGrade"] = enrollment.grade();
                course["creditHours"] = enrollment.creditHours();
                course["gpa"] = enrollment.gpa();
                courses.push_back(course);

                gradeCount++;
            }
        }

        transcript["courses"] = courses;
        transcript["cumulativeGPA"] = getStudentGPA(studentId);
        transcript["totalCourses"] = gradeCount;

        return transcript.dump();

    } catch (const std::exception& e) {
        spdlog::error("Failed to generate transcript: {}", e.what());
        throw std::runtime_error(std::string("Transcript generation failed: ") + e.what());
    }
}

bool EnrollmentService::checkPrerequisites(int64_t studentId, int64_t courseId) const {
    // Simplified prerequisite check
    // In a full implementation, would:
    // 1. Get prerequisites for course
    // 2. Check student's completed courses
    // 3. Verify student has completed all prerequisites
    // 4. Check minimum grades if required

    // For now, return true (prerequisites check deferred to Phase 5 enhancement)
    spdlog::debug("Prerequisite check for student {} in course {}", studentId, courseId);
    return true;
}

std::vector<Enrollment> EnrollmentService::getPendingGrades() const {
    return m_enrollmentRepo->getUngradedEnrollments();
}

std::string EnrollmentService::getEnrollmentStatistics(int64_t courseId) const {
    try {
        json stats;
        stats["courseId"] = courseId;

        auto enrollments = m_enrollmentRepo->findByCourse(courseId);

        int activeCount = 0;
        int completedCount = 0;
        int droppedCount = 0;
        double averageGrade = 0.0;
        int gradedCount = 0;

        for (const auto& enrollment : enrollments) {
            if (enrollment.isActive()) {
                activeCount++;
            } else if (enrollment.status() == EnrollmentStatus::COMPLETED) {
                completedCount++;
                if (enrollment.gradePosted()) {
                    averageGrade += enrollment.grade();
                    gradedCount++;
                }
            } else if (enrollment.status() == EnrollmentStatus::DROPPED) {
                droppedCount++;
            }
        }

        if (gradedCount > 0) {
            averageGrade /= gradedCount;
        }

        stats["totalEnrollments"] = enrollments.size();
        stats["activeEnrollments"] = activeCount;
        stats["completedEnrollments"] = completedCount;
        stats["droppedEnrollments"] = droppedCount;
        stats["averageGrade"] = std::round(averageGrade * 100) / 100;
        stats["gradedCount"] = gradedCount;

        return stats.dump();

    } catch (const std::exception& e) {
        spdlog::error("Failed to get enrollment statistics: {}", e.what());
        throw std::runtime_error(std::string("Statistics failed: ") + e.what());
    }
}

} // namespace sms::app::enrollments::services
