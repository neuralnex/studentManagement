#pragma once

#include "IEnrollmentService.hpp"
#include <memory>

namespace sms::app::enrollments::services {

/**
 * @brief Enrollment service implementation
 */
class EnrollmentService : public IEnrollmentService {
public:
    /**
     * @brief Constructor
     * @param enrollmentRepo Enrollment repository
     * @param courseRepo Course repository (optional, for validation)
     * @param studentRepo Student repository (optional, for validation)
     */
    EnrollmentService(
        std::shared_ptr<IEnrollmentRepository> enrollmentRepo,
        std::shared_ptr<sms::app::courses::repositories::ICourseRepository> courseRepo = nullptr,
        std::shared_ptr<sms::app::students::repositories::IStudentRepository> studentRepo = nullptr
    );

    Enrollment enrollStudent(
        int64_t studentId,
        int64_t courseId,
        const std::string& semester,
        int year
    ) override;

    std::vector<Enrollment> getStudentEnrollments(int64_t studentId) const override;

    std::vector<Enrollment> getCourseEnrollments(int64_t courseId) const override;

    void dropCourse(int64_t enrollmentId) override;

    void withdrawFromCourse(int64_t enrollmentId) override;

    void approveEnrollment(int64_t enrollmentId) override;

    void activateEnrollment(int64_t enrollmentId) override;

    void completeEnrollment(int64_t enrollmentId) override;

    void postGrade(int64_t enrollmentId, double numericalGrade) override;

    double getStudentGPA(int64_t studentId) const override;

    std::string generateTranscript(int64_t studentId) const override;

    bool checkPrerequisites(int64_t studentId, int64_t courseId) const override;

    std::vector<Enrollment> getPendingGrades() const override;

    std::string getEnrollmentStatistics(int64_t courseId) const override;

private:
    std::shared_ptr<IEnrollmentRepository> m_enrollmentRepo;
    std::shared_ptr<sms::app::courses::repositories::ICourseRepository> m_courseRepo;
    std::shared_ptr<sms::app::students::repositories::IStudentRepository> m_studentRepo;

    static std::string gradeToLetter(double numericGrade);
};

} // namespace sms::app::enrollments::services
