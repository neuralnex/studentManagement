#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QStatusBar>
#include <memory>

#include "app/authentication/services/IAuthenticationService.hpp"
#include "app/students/services/IStudentService.hpp"
#include "app/courses/repositories/ICourseRepository.hpp"
#include "app/enrollments/services/IEnrollmentService.hpp"

// Forward-declare page widgets to keep this header light
namespace sms::app::gui {
class DashboardPage;
class StudentsPage;
class CoursesPage;
} // namespace sms::app::gui

namespace sms::app::gui {

using sms::app::authentication::services::IAuthenticationService;
using sms::app::authentication::domain::User;
using sms::app::students::services::IStudentService;
using sms::app::courses::repositories::ICourseRepository;
using sms::app::enrollments::services::IEnrollmentService;

/**
 * @brief Main application window
 *
 * Hosts a left sidebar for navigation and a right stacked widget for page
 * content.  Role-based gating hides pages that the logged-in user should not
 * see.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Construct the main window
     * @param auth        Authentication service
     * @param students    Student service
     * @param courses     Course repository
     * @param enrollments Enrollment service
     * @param user        Currently logged-in user (with role populated)
     * @param sessionToken Active session token
     * @param parent      Optional Qt parent
     */
    explicit MainWindow(
        std::shared_ptr<IAuthenticationService> auth,
        std::shared_ptr<IStudentService>        students,
        std::shared_ptr<ICourseRepository>      courses,
        std::shared_ptr<IEnrollmentService>     enrollments,
        const User&                             user,
        const std::string&                      sessionToken,
        QWidget*                                parent = nullptr
    );

    ~MainWindow() override = default;

signals:
    /// Emitted when the user successfully logs out — the Application class
    /// connects this to show the login dialog again.
    void logoutRequested();

private slots:
    void onNavItemClicked(int row);
    void onLogout();

private:
    void setupMenuBar();
    void setupSidebar();
    void setupPages();

    // ── Services / session ──────────────────────────────────────────────────
    std::shared_ptr<IAuthenticationService> m_auth;
    std::shared_ptr<IStudentService>        m_students;
    std::shared_ptr<ICourseRepository>      m_courses;
    std::shared_ptr<IEnrollmentService>     m_enrollments;
    User                                    m_user;
    std::string                             m_sessionToken;

    // ── Layout widgets ───────────────────────────────────────────────────────
    QListWidget*    m_sidebar{nullptr};
    QStackedWidget* m_stack{nullptr};

    // ── Page widgets (owned by the stacked widget) ──────────────────────────
    // Admin pages
    DashboardPage* m_dashboardPage{nullptr};
    StudentsPage*  m_studentsPage{nullptr};
    CoursesPage*   m_coursesPage{nullptr};
    QWidget*       m_enrollmentsPage{nullptr};
    QWidget*       m_reportsPage{nullptr};
};

} // namespace sms::app::gui
