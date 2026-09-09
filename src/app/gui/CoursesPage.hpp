#pragma once

#include "app/courses/repositories/ICourseRepository.hpp"

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <vector>

namespace sms::app::gui {

/**
 * @brief Course management page.
 *
 * Lists all courses, supports search, add, view, and inline edit.
 */
class CoursesPage : public QWidget
{
    Q_OBJECT

public:
    explicit CoursesPage(
        sms::app::courses::repositories::ICourseRepository* courseRepo,
        QWidget* parent = nullptr
    );

public slots:
    /** Reload the table via readAll(). */
    void refresh();

private slots:
    void onSearchTextChanged(const QString& text);
    void onAddCourse();
    void onViewCourse();
    void onEditCourse();

private:
    // ── helpers ──────────────────────────────────────────────────────────────
    void populateTable(const std::vector<sms::app::courses::domain::Course>& courses);
    int64_t selectedCourseId() const;

    /** Open the course form dialog.  If editing, pre-fill from course. */
    void openCourseForm(sms::app::courses::domain::Course* existing = nullptr);

    static QString courseStatusString(sms::app::courses::domain::CourseStatus s);
    static QString semesterString(sms::app::courses::domain::Semester s);

    // ── repository ────────────────────────────────────────────────────────────
    sms::app::courses::repositories::ICourseRepository* m_courseRepo;

    // ── UI ───────────────────────────────────────────────────────────────────
    QLineEdit*    m_searchBar{nullptr};
    QTableWidget* m_table{nullptr};
    QPushButton*  m_viewBtn{nullptr};
    QPushButton*  m_editBtn{nullptr};
    QLabel*       m_statusLabel{nullptr};

    // Local cache used by search
    std::vector<sms::app::courses::domain::Course> m_allCourses;
};

} // namespace sms::app::gui
