#include "CoursesPage.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTableWidgetItem>
#include <QFont>
#include <QAbstractItemView>
#include <algorithm>

using namespace sms::app::courses::domain;

namespace sms::app::gui {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
QString CoursesPage::courseStatusString(CourseStatus s)
{
    switch (s) {
        case CourseStatus::DRAFT:     return "Draft";
        case CourseStatus::ACTIVE:    return "Active";
        case CourseStatus::ARCHIVED:  return "Archived";
        case CourseStatus::CANCELLED: return "Cancelled";
    }
    return "Unknown";
}

QString CoursesPage::semesterString(Semester s)
{
    switch (s) {
        case Semester::FALL:   return "Fall";
        case Semester::SPRING: return "Spring";
        case Semester::SUMMER: return "Summer";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
CoursesPage::CoursesPage(
    sms::app::courses::repositories::ICourseRepository* courseRepo,
    QWidget* parent
)
    : QWidget(parent)
    , m_courseRepo(courseRepo)
{
    setObjectName("CoursesPage");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    // ── Top bar ──────────────────────────────────────────────────────────────
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(10);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Search by code or title...");
    m_searchBar->setMinimumHeight(36);
    m_searchBar->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #bdc3c7; border-radius: 6px;"
        "  padding: 0 10px; font-size: 13px;"
        "}"
        "QLineEdit:focus { border-color: #3498db; }"
    );

    auto* addBtn = new QPushButton("+ Add Course", this);
    addBtn->setFixedHeight(36);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white;"
        " border: none; border-radius: 6px; padding: 0 18px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );

    auto* refreshBtn = new QPushButton("🔄", this);
    refreshBtn->setFixedSize(36, 36);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setToolTip("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { border: 1px solid #bdc3c7; border-radius: 6px; font-size: 16px; }"
        "QPushButton:hover { background-color: #ecf0f1; }"
    );

    topBar->addWidget(m_searchBar, 1);
    topBar->addWidget(addBtn);
    topBar->addWidget(refreshBtn);

    // ── Table ─────────────────────────────────────────────────────────────────
    m_table = new QTableWidget(0, 8, this);
    m_table->setHorizontalHeaderLabels(
        {"ID", "Code", "Title", "Credits", "Capacity", "Enrolled", "Semester", "Status"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setShowGrid(false);
    m_table->setStyleSheet(
        "QTableWidget { border: 1px solid #e0e0e0; border-radius: 6px; }"
        "QTableWidget::item { padding: 6px 8px; }"
        "QTableWidget::item:selected { background-color: #d0e8fb; color: #2c3e50; }"
        "QHeaderView::section {"
        "  background-color: #f5f6fa; font-weight: bold; color: #2c3e50;"
        "  border: none; border-bottom: 2px solid #e0e0e0; padding: 8px;"
        "}"
    );

    // ── Action bar ────────────────────────────────────────────────────────────
    auto* actionBar = new QHBoxLayout();
    actionBar->setSpacing(10);

    auto makeBtn = [&](const QString& text, const QString& color) -> QPushButton* {
        auto* btn = new QPushButton(text, this);
        btn->setFixedHeight(34);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QString("QPushButton { background-color: %1; color: white;"
                    " border: none; border-radius: 5px; padding: 0 14px; font-size: 13px; }"
                    "QPushButton:disabled { background-color: #bdc3c7; }").arg(color)
        );
        return btn;
    };

    m_viewBtn = makeBtn("View Details", "#3498db");
    m_editBtn = makeBtn("Edit Course",  "#e67e22");

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #7f8c8d; font-size: 12px;");

    actionBar->addWidget(m_viewBtn);
    actionBar->addWidget(m_editBtn);
    actionBar->addStretch();
    actionBar->addWidget(m_statusLabel);

    // ── Assemble ──────────────────────────────────────────────────────────────
    layout->addLayout(topBar);
    layout->addWidget(m_table, 1);
    layout->addLayout(actionBar);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_searchBar, &QLineEdit::textChanged, this, &CoursesPage::onSearchTextChanged);
    connect(addBtn,      &QPushButton::clicked,   this, &CoursesPage::onAddCourse);
    connect(refreshBtn,  &QPushButton::clicked,   this, &CoursesPage::refresh);
    connect(m_viewBtn,   &QPushButton::clicked,   this, &CoursesPage::onViewCourse);
    connect(m_editBtn,   &QPushButton::clicked,   this, &CoursesPage::onEditCourse);

    refresh();
}

// ─────────────────────────────────────────────────────────────────────────────
// populateTable
// ─────────────────────────────────────────────────────────────────────────────
void CoursesPage::populateTable(const std::vector<Course>& courses)
{
    m_table->setRowCount(0);
    for (const auto& c : courses) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto setCell = [&](int col, const QString& text, int align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            m_table->setItem(row, col, item);
        };

        setCell(0, QString::number(c.id()),            Qt::AlignCenter);
        setCell(1, QString::fromStdString(c.code()));
        setCell(2, QString::fromStdString(c.title()));
        setCell(3, QString::number(c.creditUnits()),   Qt::AlignCenter);
        setCell(4, QString::number(c.capacity()),      Qt::AlignCenter);
        setCell(5, QString::number(c.enrolledCount()), Qt::AlignCenter);
        setCell(6, semesterString(c.semester()),       Qt::AlignCenter);
        setCell(7, courseStatusString(c.status()),     Qt::AlignCenter);

        // Color-code status
        auto* statusItem = m_table->item(row, 7);
        if (statusItem) {
            switch (c.status()) {
                case CourseStatus::ACTIVE:
                    statusItem->setForeground(QColor("#27ae60")); break;
                case CourseStatus::CANCELLED:
                    statusItem->setForeground(QColor("#e74c3c")); break;
                case CourseStatus::ARCHIVED:
                    statusItem->setForeground(QColor("#7f8c8d")); break;
                case CourseStatus::DRAFT:
                    statusItem->setForeground(QColor("#e67e22")); break;
            }
        }
    }
    m_statusLabel->setText(QString("%1 course(s) shown").arg(courses.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
// refresh
// ─────────────────────────────────────────────────────────────────────────────
void CoursesPage::refresh()
{
    try {
        if (!m_courseRepo) return;
        m_allCourses = m_courseRepo->readAll();
        populateTable(m_allCourses);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// onSearchTextChanged
// ─────────────────────────────────────────────────────────────────────────────
void CoursesPage::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        populateTable(m_allCourses);
        return;
    }
    if (!m_courseRepo) return;
    try {
        if (text.length() >= 2) {
            auto results = m_courseRepo->search(text.toStdString());
            populateTable(results);
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Search Error", e.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// selectedCourseId
// ─────────────────────────────────────────────────────────────────────────────
int64_t CoursesPage::selectedCourseId() const
{
    int row = m_table->currentRow();
    if (row < 0) return -1;
    auto* item = m_table->item(row, 0);
    if (!item) return -1;
    return item->text().toLongLong();
}

// ─────────────────────────────────────────────────────────────────────────────
// openCourseForm  (shared by Add and Edit)
// ─────────────────────────────────────────────────────────────────────────────
void CoursesPage::openCourseForm(Course* existing)
{
    bool isEdit = (existing != nullptr);

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(isEdit ? "Edit Course" : "Add Course");
    dlg->setModal(true);
    dlg->setMinimumWidth(460);

    auto* form = new QFormLayout(dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    auto* codeEdit  = new QLineEdit(dlg);
    auto* titleEdit = new QLineEdit(dlg);
    auto* descEdit  = new QTextEdit(dlg);
    descEdit->setMaximumHeight(80);

    auto* creditSpin = new QSpinBox(dlg);
    creditSpin->setRange(1, 12);

    auto* capSpin = new QSpinBox(dlg);
    capSpin->setRange(1, 9999);

    auto* semCombo = new QComboBox(dlg);
    semCombo->addItems({"FALL", "SPRING", "SUMMER"});

    auto* yearSpin = new QSpinBox(dlg);
    yearSpin->setRange(2020, 2030);
    yearSpin->setValue(2026);

    auto* statusCombo = new QComboBox(dlg);
    statusCombo->addItems({"DRAFT", "ACTIVE", "ARCHIVED", "CANCELLED"});

    auto* deptSpin = new QSpinBox(dlg);
    deptSpin->setRange(1, INT_MAX);

    auto* lectSpin = new QSpinBox(dlg);
    lectSpin->setRange(0, INT_MAX);
    lectSpin->setSpecialValueText("(none)");

    // Pre-fill if editing
    if (isEdit) {
        codeEdit->setText(QString::fromStdString(existing->code()));
        titleEdit->setText(QString::fromStdString(existing->title()));
        descEdit->setPlainText(QString::fromStdString(existing->description()));
        creditSpin->setValue(existing->creditUnits());
        capSpin->setValue(existing->capacity());

        int semIdx = static_cast<int>(existing->semester());
        semCombo->setCurrentIndex(semIdx);

        yearSpin->setValue(existing->year());

        int statusIdx = static_cast<int>(existing->status());
        statusCombo->setCurrentIndex(statusIdx);

        deptSpin->setValue(static_cast<int>(existing->departmentId()));
        lectSpin->setValue(static_cast<int>(existing->lecturerId()));
    }

    form->addRow("Code:",        codeEdit);
    form->addRow("Title:",       titleEdit);
    form->addRow("Description:", descEdit);
    form->addRow("Credits:",     creditSpin);
    form->addRow("Capacity:",    capSpin);
    form->addRow("Semester:",    semCombo);
    form->addRow("Year:",        yearSpin);
    form->addRow("Status:",      statusCombo);
    form->addRow("Department ID:", deptSpin);
    form->addRow("Lecturer ID:",   lectSpin);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    buttons->button(QDialogButtonBox::Ok)->setText(isEdit ? "Save" : "Create");
    buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none;"
        " border-radius: 5px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dlg,
            [&, dlg, isEdit, existing,
             codeEdit, titleEdit, descEdit,
             creditSpin, capSpin, semCombo, yearSpin,
             statusCombo, deptSpin, lectSpin]()
    {
        if (codeEdit->text().trimmed().isEmpty() || titleEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(dlg, "Validation", "Code and Title are required.");
            return;
        }
        try {
            Course c;
            if (isEdit) {
                c = *existing;
            }

            c.setCode(codeEdit->text().trimmed().toStdString());
            c.setTitle(titleEdit->text().trimmed().toStdString());
            c.setDescription(descEdit->toPlainText().trimmed().toStdString());
            c.setCreditUnits(creditSpin->value());
            c.setCapacity(capSpin->value());

            static const Semester semMap[] = {Semester::FALL, Semester::SPRING, Semester::SUMMER};
            c.setSemester(semMap[semCombo->currentIndex()]);
            c.setYear(yearSpin->value());

            static const CourseStatus statMap[] = {
                CourseStatus::DRAFT, CourseStatus::ACTIVE,
                CourseStatus::ARCHIVED, CourseStatus::CANCELLED
            };
            c.setStatus(statMap[statusCombo->currentIndex()]);
            c.setDepartmentId(static_cast<int64_t>(deptSpin->value()));
            c.setLecturerId(static_cast<int64_t>(lectSpin->value()));

            if (isEdit) {
                m_courseRepo->update(c);
                QMessageBox::information(dlg, "Success", "Course updated successfully.");
            } else {
                m_courseRepo->create(c);
                QMessageBox::information(dlg, "Success", "Course created successfully.");
            }
            dlg->accept();
            refresh();
        } catch (const std::exception& e) {
            QMessageBox::critical(dlg, "Error", e.what());
        }
    });
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    dlg->exec();
    dlg->deleteLater();
}

// ─────────────────────────────────────────────────────────────────────────────
// onAddCourse
// ─────────────────────────────────────────────────────────────────────────────
void CoursesPage::onAddCourse()
{
    openCourseForm(nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// onViewCourse
// ─────────────────────────────────────────────────────────────────────────────
void CoursesPage::onViewCourse()
{
    int64_t cid = selectedCourseId();
    if (cid < 0) {
        QMessageBox::information(this, "No Selection", "Please select a course first.");
        return;
    }
    try {
        auto opt = m_courseRepo->read(cid);
        if (!opt) {
            QMessageBox::warning(this, "Not Found", "Course not found.");
            return;
        }
        const auto& c = *opt;

        auto* dlg = new QDialog(this);
        dlg->setWindowTitle(QString("Course Details — %1")
                            .arg(QString::fromStdString(c.code())));
        dlg->setModal(true);
        dlg->setMinimumWidth(460);

        auto* form = new QFormLayout(dlg);
        form->setContentsMargins(24, 24, 24, 24);
        form->setSpacing(10);

        auto addRow = [&](const QString& label, const QString& value) {
            auto* val = new QLabel(value.isEmpty() ? "—" : value, dlg);
            val->setWordWrap(true);
            form->addRow(label + ":", val);
        };

        addRow("ID",           QString::number(c.id()));
        addRow("Code",         QString::fromStdString(c.code()));
        addRow("Title",        QString::fromStdString(c.title()));
        addRow("Description",  QString::fromStdString(c.description()));
        addRow("Credits",      QString::number(c.creditUnits()));
        addRow("Capacity",     QString::number(c.capacity()));
        addRow("Enrolled",     QString::number(c.enrolledCount()));
        addRow("Available",    QString::number(c.getAvailableSlots()));
        addRow("Semester",     semesterString(c.semester()));
        addRow("Year",         QString::number(c.year()));
        addRow("Status",       courseStatusString(c.status()));
        addRow("Department ID",QString::number(c.departmentId()));
        addRow("Lecturer ID",  QString::number(c.lecturerId()));
        addRow("Location",     QString::fromStdString(c.location()));
        addRow("Schedule",     QString::fromStdString(c.schedule()));

        auto* closeBtn = new QPushButton("Close", dlg);
        closeBtn->setStyleSheet(
            "QPushButton { background-color: #3498db; color: white; border: none;"
            " border-radius: 5px; padding: 6px 20px; }"
            "QPushButton:hover { background-color: #2980b9; }"
        );
        form->addRow("", closeBtn);
        connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

        dlg->exec();
        dlg->deleteLater();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// onEditCourse
// ─────────────────────────────────────────────────────────────────────────────
void CoursesPage::onEditCourse()
{
    int64_t cid = selectedCourseId();
    if (cid < 0) {
        QMessageBox::information(this, "No Selection", "Please select a course first.");
        return;
    }
    try {
        auto opt = m_courseRepo->read(cid);
        if (!opt) {
            QMessageBox::warning(this, "Not Found", "Course not found.");
            return;
        }
        Course c = *opt;
        openCourseForm(&c);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

} // namespace sms::app::gui
