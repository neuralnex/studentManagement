#include "Application.hpp"

#include <QApplication>
#include <QString>

#include <spdlog/spdlog.h>

#include "LoginDialog.hpp"
#include "MainWindow.hpp"

namespace sms::app::gui {

// ─── Global stylesheet applied to QApplication ──────────────────────────────

static constexpr const char* GLOBAL_STYLE = R"(
QWidget {
    font-family: 'Segoe UI', Arial, sans-serif;
    font-size: 13px;
}
QMainWindow {
    background-color: #f5f6fa;
}
QPushButton {
    padding: 6px 16px;
    border-radius: 4px;
}
QLineEdit {
    border: 1px solid #dcdde1;
    border-radius: 4px;
    padding: 6px 10px;
    background: white;
}
QLineEdit:focus {
    border-color: #3498db;
}
QTableWidget {
    gridline-color: #dcdde1;
    background: white;
    alternate-background-color: #f8f9fa;
}
QHeaderView::section {
    background-color: #2c3e50;
    color: white;
    padding: 8px;
    border: none;
    font-weight: bold;
}
QGroupBox {
    border: 1px solid #dcdde1;
    border-radius: 6px;
    margin-top: 10px;
    font-weight: bold;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 5px;
}
)";

// ─── Constructor ─────────────────────────────────────────────────────────────

Application::Application(
    std::shared_ptr<IAuthenticationService> auth,
    std::shared_ptr<IStudentService>        students,
    std::shared_ptr<ICourseRepository>      courses,
    std::shared_ptr<IEnrollmentService>     enrollments
)
    : m_auth(std::move(auth))
    , m_students(std::move(students))
    , m_courses(std::move(courses))
    , m_enrollments(std::move(enrollments))
{}

// ─── run ─────────────────────────────────────────────────────────────────────

int Application::run(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Student Management System"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("University of Excellence"));
    app.setOrganizationDomain(QStringLiteral("university.edu"));
    app.setStyleSheet(QLatin1String(GLOBAL_STYLE));

    // Keep the event loop alive even when the main window is hidden
    // (so we can re-show the login dialog after logout).
    app.setQuitOnLastWindowClosed(false);

    // Capture services by value so the lambda closures below stay valid
    // across multiple login/logout cycles.
    auto auth        = m_auth;
    auto students    = m_students;
    auto courses     = m_courses;
    auto enrollments = m_enrollments;

    // We hold a raw pointer to the MainWindow so we can delete and re-create
    // it on each successful login without leaking.
    MainWindow* mainWindow = nullptr;

    // ── Login / main-window loop ─────────────────────────────────────────────
    // This lambda is called once at startup and then again every time
    // MainWindow emits logoutRequested().
    std::function<void()> showLogin;
    showLogin = [&]() {
        // Destroy the previous main window (if any) before showing login.
        if (mainWindow) {
            mainWindow->deleteLater();
            mainWindow = nullptr;
        }

        auto* dialog = new LoginDialog(auth);
        const int result = dialog->exec();

        if (result != QDialog::Accepted) {
            // User closed the dialog without logging in — quit.
            spdlog::info("Login dialog closed without login — quitting.");
            app.quit();
            dialog->deleteLater();
            return;
        }

        const auto& lr = dialog->loginResult();
        dialog->deleteLater();

        spdlog::info(
            "Creating MainWindow for user '{}' (role: {})",
            lr.user.email(),
            lr.user.role().name()
        );

        mainWindow = new MainWindow(
            auth, students, courses, enrollments,
            lr.user, lr.sessionToken
        );

        // When the user logs out, come back to the login dialog.
        QObject::connect(
            mainWindow, &MainWindow::logoutRequested,
            [&showLogin]() { showLogin(); }
        );

        mainWindow->show();
    };

    showLogin();

    return app.exec();
}

} // namespace sms::app::gui
