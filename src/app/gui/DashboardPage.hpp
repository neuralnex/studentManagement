#pragma once

#include "app/authentication/services/IAuthenticationService.hpp"
#include "app/students/services/IStudentService.hpp"
#include "app/courses/repositories/ICourseRepository.hpp"
#include "app/enrollments/services/IEnrollmentService.hpp"

#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QPushButton>

namespace sms::app::gui {

/**
 * @brief Dashboard page widget
 *
 * Displays a 2x2 stat card grid: Total Students, Active Students,
 * Total Courses, Pending Grades.  Cards refresh on demand.
 */
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(
        sms::app::authentication::services::IAuthenticationService* authService,
        sms::app::students::services::IStudentService*               studentService,
        sms::app::courses::repositories::ICourseRepository*          courseRepo,
        sms::app::enrollments::services::IEnrollmentService*         enrollmentService,
        const QString& currentUserFirstName = "Admin",
        QWidget* parent = nullptr
    );

public slots:
    /** Fetch counts from services and update the stat labels. */
    void refresh();

private:
    // ── helpers ──────────────────────────────────────────────────────────────
    QFrame* makeCard(const QString& accentColor,
                     const QString& icon,
                     const QString& label,
                     QLabel*&       countLabelOut);

    // ── services ─────────────────────────────────────────────────────────────
    sms::app::authentication::services::IAuthenticationService* m_authService;
    sms::app::students::services::IStudentService*               m_studentService;
    sms::app::courses::repositories::ICourseRepository*          m_courseRepo;
    sms::app::enrollments::services::IEnrollmentService*         m_enrollmentService;

    // ── UI ───────────────────────────────────────────────────────────────────
    QLabel* m_welcomeLabel{nullptr};

    QLabel* m_totalStudentsCount{nullptr};
    QLabel* m_activeStudentsCount{nullptr};
    QLabel* m_totalCoursesCount{nullptr};
    QLabel* m_pendingGradesCount{nullptr};
};

} // namespace sms::app::gui
