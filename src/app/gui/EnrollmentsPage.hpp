#pragma once

#include "app/enrollments/services/IEnrollmentService.hpp"

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>

namespace sms::app::gui {

/**
 * @brief Enrollment management page with 5 tabs.
 *
 * Tabs:
 *   1. Enroll Student
 *   2. Student Enrollments
 *   3. Course Enrollments
 *   4. Pending Grades
 *   5. Transcript & GPA
 */
class EnrollmentsPage : public QWidget
{
    Q_OBJECT

public:
    explicit EnrollmentsPage(
        sms::app::enrollments::services::IEnrollmentService* enrollmentService,
        QWidget* parent = nullptr
    );

private slots:
    // Tab 1
    void onEnrollStudent();

    // Tab 2
    void onLoadStudentEnrollments();
    void onApproveEnrollment();
    void onDropEnrollment();
    void onWithdrawEnrollment();
    void onCompleteEnrollment();
    void onPostGradeStudent();

    // Tab 3
    void onLoadCourseEnrollments();
    void onApproveEnrollmentCourse();
    void onDropEnrollmentCourse();
    void onWithdrawEnrollmentCourse();
    void onCompleteEnrollmentCourse();
    void onPostGradeCourse();

    // Tab 4
    void onRefreshPendingGrades();
    void onPostGradePending();

    // Tab 5
    void onLoadTranscript();

private:
    // ── helpers ──────────────────────────────────────────────────────────────
    void populateEnrollmentTable(QTableWidget* table,
                                 const std::vector<sms::app::enrollments::domain::Enrollment>& enrollments,
                                 bool showStudentId = true,
                                 bool showCourseId  = true);

    int64_t selectedEnrollmentId(QTableWidget* table) const;
    void    showPostGradeDialog(int64_t enrollmentId);

    static QString enrollmentStatusString(sms::app::enrollments::domain::EnrollmentStatus s);

    // ── service ──────────────────────────────────────────────────────────────
    sms::app::enrollments::services::IEnrollmentService* m_enrollmentService;

    // ── Tab 1 widgets ─────────────────────────────────────────────────────────
    QSpinBox*   m_enrollStudentId{nullptr};
    QSpinBox*   m_enrollCourseId{nullptr};
    QLineEdit*  m_enrollSemester{nullptr};
    QSpinBox*   m_enrollYear{nullptr};
    QLabel*     m_enrollResultLabel{nullptr};

    // ── Tab 2 widgets ─────────────────────────────────────────────────────────
    QSpinBox*     m_studentIdInput{nullptr};
    QTableWidget* m_studentEnrollTable{nullptr};

    // ── Tab 3 widgets ─────────────────────────────────────────────────────────
    QSpinBox*     m_courseIdInput{nullptr};
    QTableWidget* m_courseEnrollTable{nullptr};

    // ── Tab 4 widgets ─────────────────────────────────────────────────────────
    QTableWidget* m_pendingGradesTable{nullptr};

    // ── Tab 5 widgets ─────────────────────────────────────────────────────────
    QSpinBox*  m_transcriptStudentId{nullptr};
    QLabel*    m_gpaLabel{nullptr};
    QTextEdit* m_transcriptText{nullptr};
};

} // namespace sms::app::gui
