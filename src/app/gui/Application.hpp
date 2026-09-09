#pragma once

#include <memory>

#include "app/authentication/services/IAuthenticationService.hpp"
#include "app/students/services/IStudentService.hpp"
#include "app/courses/repositories/ICourseRepository.hpp"
#include "app/enrollments/services/IEnrollmentService.hpp"

namespace sms::app::gui {

using sms::app::authentication::services::IAuthenticationService;
using sms::app::students::services::IStudentService;
using sms::app::courses::repositories::ICourseRepository;
using sms::app::enrollments::services::IEnrollmentService;

/**
 * @brief Top-level GUI application controller
 *
 * Owns the QApplication instance and orchestrates the login/main-window loop.
 * Does NOT inherit QApplication — it constructs one internally inside run().
 */
class Application {
public:
    /**
     * @brief Construct the application controller
     * @param auth        Authentication service
     * @param students    Student service
     * @param courses     Course repository
     * @param enrollments Enrollment service
     */
    explicit Application(
        std::shared_ptr<IAuthenticationService> auth,
        std::shared_ptr<IStudentService>        students,
        std::shared_ptr<ICourseRepository>      courses,
        std::shared_ptr<IEnrollmentService>     enrollments
    );

    ~Application() = default;

    // Non-copyable, non-movable (owns Qt state)
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    /**
     * @brief Run the Qt event loop
     *
     * Creates QApplication, shows the login dialog, and on success shows the
     * main window. If the user logs out, the login dialog is shown again.
     * Returns when the last window is closed.
     *
     * @param argc  Forwarded to QApplication
     * @param argv  Forwarded to QApplication
     * @return QApplication::exec() return value, or 0 if login was cancelled
     */
    int run(int argc, char* argv[]);

private:
    std::shared_ptr<IAuthenticationService> m_auth;
    std::shared_ptr<IStudentService>        m_students;
    std::shared_ptr<ICourseRepository>      m_courses;
    std::shared_ptr<IEnrollmentService>     m_enrollments;
};

} // namespace sms::app::gui
