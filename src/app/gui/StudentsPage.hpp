#pragma once

#include "app/students/services/IStudentService.hpp"

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <vector>

namespace sms::app::gui {

/**
 * @brief Full student management page.
 *
 * Provides search, add, view, status editing, and emergency-contact
 * management for the student roster.
 */
class StudentsPage : public QWidget
{
    Q_OBJECT

public:
    explicit StudentsPage(
        sms::app::students::services::IStudentService* studentService,
        QWidget* parent = nullptr
    );

public slots:
    /** Reload the table from the service (active students, offset 0, limit 500). */
    void refresh();

private slots:
    void onSearchTextChanged(const QString& text);
    void onAddStudent();
    void onViewStudent();
    void onEditStatus();
    void onEmergencyContact();

private:
    // ── helpers ──────────────────────────────────────────────────────────────
    void populateTable(const std::vector<sms::app::students::domain::Student>& students);
    int64_t selectedStudentId() const;

    static QString studentStatusString(sms::app::students::domain::StudentStatus s);
    static QString academicStandingString(sms::app::students::domain::AcademicStanding s);

    // ── service ──────────────────────────────────────────────────────────────
    sms::app::students::services::IStudentService* m_studentService;

    // ── UI ───────────────────────────────────────────────────────────────────
    QLineEdit*    m_searchBar{nullptr};
    QTableWidget* m_table{nullptr};
    QPushButton*  m_viewBtn{nullptr};
    QPushButton*  m_editStatusBtn{nullptr};
    QPushButton*  m_emergencyBtn{nullptr};
    QLabel*       m_statusLabel{nullptr};
};

} // namespace sms::app::gui
