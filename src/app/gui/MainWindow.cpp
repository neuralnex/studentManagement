#include "MainWindow.hpp"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>

#include <spdlog/spdlog.h>

#include "DashboardPage.hpp"
#include "StudentsPage.hpp"
#include "CoursesPage.hpp"
#include "EnrollmentsPage.hpp"

namespace sms::app::gui {

// ─── Sidebar stylesheet ──────────────────────────────────────────────────────

static constexpr const char* SIDEBAR_STYLE = R"(
QListWidget {
    background-color: #2c3e50;
    color: white;
    border: none;
    font-size: 14px;
    padding: 5px;
}
QListWidget::item {
    padding: 12px 15px;
    border-radius: 5px;
}
QListWidget::item:selected {
    background-color: #3498db;
}
QListWidget::item:hover:!selected {
    background-color: #34495e;
}
)";

// ─── Helper: create a centred placeholder page ───────────────────────────────

static QWidget* makePlaceholderPage(const QString& pageName, QWidget* parent = nullptr) {
    auto* page  = new QWidget(parent);
    auto* label = new QLabel(QStringLiteral("Loading %1...").arg(pageName), page);

    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(
        "color: #7f8c8d; font-size: 18px; font-weight: bold;"
    );

    auto* layout = new QVBoxLayout(page);
    layout->addWidget(label);
    layout->setAlignment(Qt::AlignCenter);
    page->setStyleSheet("background-color: #f5f6fa;");

    return page;
}

// ─── Constructor ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(
    std::shared_ptr<IAuthenticationService> auth,
    std::shared_ptr<IStudentService>        students,
    std::shared_ptr<ICourseRepository>      courses,
    std::shared_ptr<IEnrollmentService>     enrollments,
    const User&                             user,
    const std::string&                      sessionToken,
    QWidget*                                parent
)
    : QMainWindow(parent)
    , m_auth(std::move(auth))
    , m_students(std::move(students))
    , m_courses(std::move(courses))
    , m_enrollments(std::move(enrollments))
    , m_user(user)
    , m_sessionToken(sessionToken)
{
    setWindowTitle(QStringLiteral("Student Management System v1.0.0"));
    setMinimumSize(1100, 700);

    setupMenuBar();

    // ── Central layout: sidebar | stack ────────────────────────────────────
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Sidebar
    m_sidebar = new QListWidget(centralWidget);
    m_sidebar->setFixedWidth(200);
    m_sidebar->setStyleSheet(SIDEBAR_STYLE);
    m_sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Stacked content area
    m_stack = new QStackedWidget(centralWidget);
    m_stack->setStyleSheet("background-color: #f5f6fa;");

    rootLayout->addWidget(m_sidebar);
    rootLayout->addWidget(m_stack);

    setupPages();
    setupSidebar();

    // Navigation signal
    connect(
        m_sidebar, &QListWidget::currentRowChanged,
        this,      &MainWindow::onNavItemClicked
    );

    // Select first item by default
    if (m_sidebar->count() > 0) {
        m_sidebar->setCurrentRow(0);
    }

    // Status bar
    const QString roleName = QString::fromStdString(m_user.role().name());
    const QString displayName = QString::fromStdString(
        m_user.firstName() + " " + m_user.lastName()
    ).trimmed();
    const QString identity = displayName.isEmpty()
        ? QString::fromStdString(m_user.email())
        : displayName;

    statusBar()->showMessage(
        QStringLiteral("Logged in as: %1  |  Role: %2").arg(identity, roleName)
    );

    spdlog::info(
        "MainWindow opened for user '{}' (role: {})",
        m_user.email(),
        m_user.role().name()
    );
}

// ─── setupMenuBar ────────────────────────────────────────────────────────────

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

    auto* logoutAction = new QAction(QStringLiteral("&Logout"), this);
    logoutAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
    fileMenu->addAction(logoutAction);

    fileMenu->addSeparator();

    auto* exitAction = new QAction(QStringLiteral("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);
}

// ─── setupPages ─────────────────────────────────────────────────────────────

void MainWindow::setupPages() {
    const bool isAdmin = (m_user.role().name() != "Student");

    const QString firstName = QString::fromStdString(
        m_user.firstName().empty() ? m_user.email() : m_user.firstName()
    );

    // Dashboard — visible to everyone; real implementation
    m_dashboardPage = new DashboardPage(
        m_auth.get(),
        m_students.get(),
        m_courses.get(),
        m_enrollments.get(),
        firstName
    );
    m_stack->addWidget(m_dashboardPage);

    if (isAdmin) {
        // Real Students page
        m_studentsPage = new StudentsPage(m_students.get());
        m_stack->addWidget(m_studentsPage);

        // Real Courses page
        m_coursesPage = new CoursesPage(m_courses.get());
        m_stack->addWidget(m_coursesPage);

        // Real Enrollments page
        m_enrollmentsPage = new EnrollmentsPage(m_enrollments.get());
        m_stack->addWidget(m_enrollmentsPage);

        m_reportsPage = makePlaceholderPage(QStringLiteral("Reports"));
        m_stack->addWidget(m_reportsPage);
    } else {
        // Student-role pages
        m_enrollmentsPage = new EnrollmentsPage(m_enrollments.get());
        m_stack->addWidget(m_enrollmentsPage);

        m_reportsPage = makePlaceholderPage(QStringLiteral("My Transcript"));
        m_stack->addWidget(m_reportsPage);
    }
}

// ─── setupSidebar ────────────────────────────────────────────────────────────

void MainWindow::setupSidebar() {
    const bool isAdmin = (m_user.role().name() != "Student");

    if (isAdmin) {
        m_sidebar->addItem(QStringLiteral("\U0001F3E0  Dashboard"));
        m_sidebar->addItem(QStringLiteral("\U0001F465  Students"));
        m_sidebar->addItem(QStringLiteral("\U0001F4DA  Courses"));
        m_sidebar->addItem(QStringLiteral("\U0001F4CB  Enrollments"));
        m_sidebar->addItem(QStringLiteral("\U0001F4CA  Reports"));
    } else {
        m_sidebar->addItem(QStringLiteral("\U0001F3E0  Dashboard"));
        m_sidebar->addItem(QStringLiteral("\U0001F4CB  My Enrollments"));
        m_sidebar->addItem(QStringLiteral("\U0001F4C4  My Transcript"));
    }
}

// ─── Slots ───────────────────────────────────────────────────────────────────

void MainWindow::onNavItemClicked(int row) {
    if (row < 0 || row >= m_stack->count()) {
        return;
    }
    m_stack->setCurrentIndex(row);
}

void MainWindow::onLogout() {
    try {
        m_auth->logout(m_sessionToken);
        spdlog::info("User '{}' logged out", m_user.email());
    } catch (const std::exception& ex) {
        spdlog::warn("Logout error (ignored): {}", ex.what());
    }

    hide();
    emit logoutRequested();
}

} // namespace sms::app::gui
