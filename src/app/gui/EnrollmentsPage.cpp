#include "EnrollmentsPage.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTableWidgetItem>
#include <QFont>
#include <QAbstractItemView>
#include <QGroupBox>
#include <QScrollArea>

#include <nlohmann/json.hpp>
#include <sstream>

using namespace sms::app::enrollments::domain;
using json = nlohmann::json;

namespace sms::app::gui {

// ─────────────────────────────────────────────────────────────────────────────
// Helper: status to string
// ─────────────────────────────────────────────────────────────────────────────
QString EnrollmentsPage::enrollmentStatusString(EnrollmentStatus s)
{
    switch (s) {
        case EnrollmentStatus::PENDING:   return "Pending";
        case EnrollmentStatus::APPROVED:  return "Approved";
        case EnrollmentStatus::ACTIVE:    return "Active";
        case EnrollmentStatus::COMPLETED: return "Completed";
        case EnrollmentStatus::DROPPED:   return "Dropped";
        case EnrollmentStatus::WITHDRAWN: return "Withdrawn";
        case EnrollmentStatus::FAILED:    return "Failed";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: populate a generic enrollment table
// Columns: ID | StudentID | CourseID | Semester | Year | Status | Grade | Letter
// ─────────────────────────────────────────────────────────────────────────────
void EnrollmentsPage::populateEnrollmentTable(
    QTableWidget* table,
    const std::vector<Enrollment>& enrollments,
    bool showStudentId,
    bool showCourseId)
{
    table->setRowCount(0);
    for (const auto& e : enrollments) {
        int row = table->rowCount();
        table->insertRow(row);

        int col = 0;
        auto setCell = [&](const QString& text, int align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            table->setItem(row, col++, item);
        };

        setCell(QString::number(e.id()),        Qt::AlignCenter);
        if (showStudentId) setCell(QString::number(e.studentId()), Qt::AlignCenter);
        if (showCourseId)  setCell(QString::number(e.courseId()),  Qt::AlignCenter);
        setCell(QString::fromStdString(e.semester()));
        setCell(QString::number(e.year()),      Qt::AlignCenter);
        setCell(enrollmentStatusString(e.status()), Qt::AlignCenter);

        QString gradeStr = e.gradePosted()
            ? QString::number(e.grade(), 'f', 1) : "—";
        setCell(gradeStr, Qt::AlignCenter);
        setCell(e.letterGrade().empty() ? "—"
                : QString::fromStdString(e.letterGrade()), Qt::AlignCenter);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: get selected enrollment ID from a table (ID is always col 0)
// ─────────────────────────────────────────────────────────────────────────────
int64_t EnrollmentsPage::selectedEnrollmentId(QTableWidget* table) const
{
    int row = table->currentRow();
    if (row < 0) return -1;
    auto* item = table->item(row, 0);
    if (!item) return -1;
    return item->text().toLongLong();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: open post-grade dialog
// ─────────────────────────────────────────────────────────────────────────────
void EnrollmentsPage::showPostGradeDialog(int64_t enrollmentId)
{
    if (enrollmentId < 0) {
        QMessageBox::information(this, "No Selection", "Please select an enrollment first.");
        return;
    }

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Post Grade");
    dlg->setModal(true);
    dlg->setMinimumWidth(320);

    auto* form = new QFormLayout(dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    form->addRow(new QLabel(
        QString("Enrollment ID: <b>%1</b>").arg(enrollmentId), dlg));

    auto* gradeSpin = new QDoubleSpinBox(dlg);
    gradeSpin->setRange(0.0, 100.0);
    gradeSpin->setDecimals(1);
    gradeSpin->setSingleStep(0.5);
    form->addRow("Numerical Grade (0–100):", gradeSpin);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    buttons->button(QDialogButtonBox::Ok)->setText("Post Grade");
    buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none;"
        " border-radius: 5px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dlg, [&, dlg, gradeSpin, enrollmentId]() {
        try {
            m_enrollmentService->postGrade(enrollmentId, gradeSpin->value());
            QMessageBox::information(dlg, "Success",
                QString("Grade %1 posted for enrollment %2.")
                    .arg(gradeSpin->value(), 0, 'f', 1).arg(enrollmentId));
            dlg->accept();
        } catch (const std::exception& e) {
            QMessageBox::critical(dlg, "Error", e.what());
        }
    });
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    dlg->exec();
    dlg->deleteLater();
}

// ─────────────────────────────────────────────────────────────────────────────
// Common table factory
// ─────────────────────────────────────────────────────────────────────────────
static QTableWidget* makeEnrollTable(
    int columnCount,
    const QStringList& headers,
    QWidget* parent)
{
    auto* table = new QTableWidget(0, columnCount, parent);
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setShowGrid(false);
    table->setStyleSheet(
        "QTableWidget { border: 1px solid #e0e0e0; border-radius: 6px; }"
        "QTableWidget::item { padding: 6px 8px; }"
        "QTableWidget::item:selected { background-color: #d0e8fb; color: #2c3e50; }"
        "QHeaderView::section {"
        "  background-color: #f5f6fa; font-weight: bold; color: #2c3e50;"
        "  border: none; border-bottom: 2px solid #e0e0e0; padding: 8px;"
        "}"
    );
    return table;
}

static QPushButton* makeBtn(const QString& text, const QString& color, QWidget* parent)
{
    auto* btn = new QPushButton(text, parent);
    btn->setFixedHeight(34);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        QString("QPushButton { background-color: %1; color: white;"
                " border: none; border-radius: 5px; padding: 0 14px; font-size: 13px; }"
                "QPushButton:hover { opacity: 0.9; }"
                "QPushButton:disabled { background-color: #bdc3c7; }").arg(color)
    );
    return btn;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
EnrollmentsPage::EnrollmentsPage(
    sms::app::enrollments::services::IEnrollmentService* enrollmentService,
    QWidget* parent
)
    : QWidget(parent)
    , m_enrollmentService(enrollmentService)
{
    setObjectName("EnrollmentsPage");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    auto* tabs = new QTabWidget(this);
    tabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #e0e0e0; border-radius: 6px; }"
        "QTabBar::tab { padding: 8px 20px; font-size: 13px; }"
        "QTabBar::tab:selected { color: #3498db; border-bottom: 2px solid #3498db; }"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // TAB 1: Enroll Student
    // ═══════════════════════════════════════════════════════════════════════════
    {
        auto* tab = new QWidget();
        auto* vl  = new QVBoxLayout(tab);
        vl->setContentsMargins(24, 24, 24, 24);
        vl->setSpacing(16);

        auto* group = new QGroupBox("Enroll Student in Course", tab);
        auto* form  = new QFormLayout(group);
        form->setSpacing(12);
        form->setContentsMargins(16, 20, 16, 16);

        m_enrollStudentId = new QSpinBox(group);
        m_enrollStudentId->setRange(1, INT_MAX);

        m_enrollCourseId = new QSpinBox(group);
        m_enrollCourseId->setRange(1, INT_MAX);

        m_enrollSemester = new QLineEdit(group);
        m_enrollSemester->setPlaceholderText("e.g. FALL2026");

        m_enrollYear = new QSpinBox(group);
        m_enrollYear->setRange(2020, 2030);
        m_enrollYear->setValue(2026);

        form->addRow("Student ID:", m_enrollStudentId);
        form->addRow("Course ID:",  m_enrollCourseId);
        form->addRow("Semester:",   m_enrollSemester);
        form->addRow("Year:",       m_enrollYear);

        auto* enrollBtn = makeBtn("Enroll", "#3498db", group);
        form->addRow("", enrollBtn);

        m_enrollResultLabel = new QLabel("", tab);
        m_enrollResultLabel->setWordWrap(true);
        m_enrollResultLabel->setStyleSheet("font-size: 13px; padding: 8px 0;");

        vl->addWidget(group);
        vl->addWidget(m_enrollResultLabel);
        vl->addStretch();

        connect(enrollBtn, &QPushButton::clicked, this, &EnrollmentsPage::onEnrollStudent);
        tabs->addTab(tab, "Enroll Student");
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // TAB 2: Student Enrollments
    // ═══════════════════════════════════════════════════════════════════════════
    {
        auto* tab = new QWidget();
        auto* vl  = new QVBoxLayout(tab);
        vl->setContentsMargins(20, 20, 20, 20);
        vl->setSpacing(12);

        // Search bar
        auto* topBar = new QHBoxLayout();
        auto* sidLabel = new QLabel("Student ID:", tab);
        m_studentIdInput = new QSpinBox(tab);
        m_studentIdInput->setRange(1, INT_MAX);
        m_studentIdInput->setFixedWidth(120);
        auto* loadBtn = makeBtn("Load", "#3498db", tab);
        loadBtn->setFixedWidth(80);

        topBar->addWidget(sidLabel);
        topBar->addWidget(m_studentIdInput);
        topBar->addWidget(loadBtn);
        topBar->addStretch();

        // Table: ID | Course ID | Semester | Year | Status | Grade | Letter
        m_studentEnrollTable = makeEnrollTable(7,
            {"ID", "Course ID", "Semester", "Year", "Status", "Grade", "Letter"},
            tab);

        // Actions
        auto* actionBar = new QHBoxLayout();
        actionBar->setSpacing(8);
        auto* approveBtn  = makeBtn("Approve",    "#27ae60", tab);
        auto* dropBtn     = makeBtn("Drop",       "#e74c3c", tab);
        auto* withdrawBtn = makeBtn("Withdraw",   "#e67e22", tab);
        auto* completeBtn = makeBtn("Complete",   "#3498db", tab);
        auto* postGradeBtn= makeBtn("Post Grade", "#9b59b6", tab);
        actionBar->addWidget(approveBtn);
        actionBar->addWidget(dropBtn);
        actionBar->addWidget(withdrawBtn);
        actionBar->addWidget(completeBtn);
        actionBar->addWidget(postGradeBtn);
        actionBar->addStretch();

        vl->addLayout(topBar);
        vl->addWidget(m_studentEnrollTable, 1);
        vl->addLayout(actionBar);

        connect(loadBtn,     &QPushButton::clicked, this, &EnrollmentsPage::onLoadStudentEnrollments);
        connect(approveBtn,  &QPushButton::clicked, this, &EnrollmentsPage::onApproveEnrollment);
        connect(dropBtn,     &QPushButton::clicked, this, &EnrollmentsPage::onDropEnrollment);
        connect(withdrawBtn, &QPushButton::clicked, this, &EnrollmentsPage::onWithdrawEnrollment);
        connect(completeBtn, &QPushButton::clicked, this, &EnrollmentsPage::onCompleteEnrollment);
        connect(postGradeBtn,&QPushButton::clicked, this, &EnrollmentsPage::onPostGradeStudent);

        tabs->addTab(tab, "Student Enrollments");
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // TAB 3: Course Enrollments
    // ═══════════════════════════════════════════════════════════════════════════
    {
        auto* tab = new QWidget();
        auto* vl  = new QVBoxLayout(tab);
        vl->setContentsMargins(20, 20, 20, 20);
        vl->setSpacing(12);

        auto* topBar = new QHBoxLayout();
        auto* cidLabel = new QLabel("Course ID:", tab);
        m_courseIdInput = new QSpinBox(tab);
        m_courseIdInput->setRange(1, INT_MAX);
        m_courseIdInput->setFixedWidth(120);
        auto* loadBtn = makeBtn("Load", "#3498db", tab);
        loadBtn->setFixedWidth(80);
        topBar->addWidget(cidLabel);
        topBar->addWidget(m_courseIdInput);
        topBar->addWidget(loadBtn);
        topBar->addStretch();

        // Table: ID | Student ID | Semester | Year | Status | Grade | Letter
        m_courseEnrollTable = makeEnrollTable(7,
            {"ID", "Student ID", "Semester", "Year", "Status", "Grade", "Letter"},
            tab);

        auto* actionBar = new QHBoxLayout();
        actionBar->setSpacing(8);
        auto* approveBtn  = makeBtn("Approve",    "#27ae60", tab);
        auto* dropBtn     = makeBtn("Drop",       "#e74c3c", tab);
        auto* withdrawBtn = makeBtn("Withdraw",   "#e67e22", tab);
        auto* completeBtn = makeBtn("Complete",   "#3498db", tab);
        auto* postGradeBtn= makeBtn("Post Grade", "#9b59b6", tab);
        actionBar->addWidget(approveBtn);
        actionBar->addWidget(dropBtn);
        actionBar->addWidget(withdrawBtn);
        actionBar->addWidget(completeBtn);
        actionBar->addWidget(postGradeBtn);
        actionBar->addStretch();

        vl->addLayout(topBar);
        vl->addWidget(m_courseEnrollTable, 1);
        vl->addLayout(actionBar);

        connect(loadBtn,     &QPushButton::clicked, this, &EnrollmentsPage::onLoadCourseEnrollments);
        connect(approveBtn,  &QPushButton::clicked, this, &EnrollmentsPage::onApproveEnrollmentCourse);
        connect(dropBtn,     &QPushButton::clicked, this, &EnrollmentsPage::onDropEnrollmentCourse);
        connect(withdrawBtn, &QPushButton::clicked, this, &EnrollmentsPage::onWithdrawEnrollmentCourse);
        connect(completeBtn, &QPushButton::clicked, this, &EnrollmentsPage::onCompleteEnrollmentCourse);
        connect(postGradeBtn,&QPushButton::clicked, this, &EnrollmentsPage::onPostGradeCourse);

        tabs->addTab(tab, "Course Enrollments");
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // TAB 4: Pending Grades
    // ═══════════════════════════════════════════════════════════════════════════
    {
        auto* tab = new QWidget();
        auto* vl  = new QVBoxLayout(tab);
        vl->setContentsMargins(20, 20, 20, 20);
        vl->setSpacing(12);

        auto* topBar = new QHBoxLayout();
        auto* refreshBtn = makeBtn("🔄 Refresh", "#3498db", tab);
        refreshBtn->setFixedWidth(120);
        auto* postGradeBtn = makeBtn("Post Grade", "#9b59b6", tab);
        topBar->addWidget(refreshBtn);
        topBar->addWidget(postGradeBtn);
        topBar->addStretch();

        // Table: ID | Student ID | Course ID | Semester | Year | Status | Grade | Letter
        m_pendingGradesTable = makeEnrollTable(8,
            {"ID", "Student ID", "Course ID", "Semester", "Year", "Status", "Grade", "Letter"},
            tab);

        vl->addLayout(topBar);
        vl->addWidget(m_pendingGradesTable, 1);

        connect(refreshBtn,  &QPushButton::clicked, this, &EnrollmentsPage::onRefreshPendingGrades);
        connect(postGradeBtn,&QPushButton::clicked, this, &EnrollmentsPage::onPostGradePending);

        tabs->addTab(tab, "Pending Grades");
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // TAB 5: Transcript & GPA
    // ═══════════════════════════════════════════════════════════════════════════
    {
        auto* tab = new QWidget();
        auto* vl  = new QVBoxLayout(tab);
        vl->setContentsMargins(24, 24, 24, 24);
        vl->setSpacing(16);

        // Input row
        auto* topBar = new QHBoxLayout();
        auto* label  = new QLabel("Student ID:", tab);
        m_transcriptStudentId = new QSpinBox(tab);
        m_transcriptStudentId->setRange(1, INT_MAX);
        m_transcriptStudentId->setFixedWidth(120);
        auto* loadBtn = makeBtn("Load Transcript", "#3498db", tab);
        topBar->addWidget(label);
        topBar->addWidget(m_transcriptStudentId);
        topBar->addWidget(loadBtn);
        topBar->addStretch();

        // GPA display
        auto* gpaRow = new QHBoxLayout();
        auto* gpaTitle = new QLabel("GPA:", tab);
        QFont gpaFont;
        gpaFont.setPointSize(14);
        gpaFont.setBold(true);
        gpaTitle->setFont(gpaFont);

        m_gpaLabel = new QLabel("—", tab);
        QFont gpaValueFont;
        gpaValueFont.setPointSize(28);
        gpaValueFont.setBold(true);
        m_gpaLabel->setFont(gpaValueFont);
        m_gpaLabel->setStyleSheet("color: #3498db;");

        gpaRow->addWidget(gpaTitle, 0, Qt::AlignVCenter);
        gpaRow->addWidget(m_gpaLabel, 0, Qt::AlignVCenter);
        gpaRow->addStretch();

        // Transcript display
        m_transcriptText = new QTextEdit(tab);
        m_transcriptText->setReadOnly(true);
        m_transcriptText->setPlaceholderText("Transcript will appear here...");
        m_transcriptText->setStyleSheet(
            "QTextEdit { font-family: monospace; font-size: 13px;"
            " border: 1px solid #e0e0e0; border-radius: 6px; padding: 8px; }"
        );

        vl->addLayout(topBar);
        vl->addLayout(gpaRow);
        vl->addWidget(m_transcriptText, 1);

        connect(loadBtn, &QPushButton::clicked, this, &EnrollmentsPage::onLoadTranscript);

        tabs->addTab(tab, "Transcript & GPA");
    }

    mainLayout->addWidget(tabs);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab 1 slots
// ─────────────────────────────────────────────────────────────────────────────
void EnrollmentsPage::onEnrollStudent()
{
    if (!m_enrollmentService) return;
    if (m_enrollSemester->text().trimmed().isEmpty()) {
        m_enrollResultLabel->setText("<span style='color:#e74c3c;'>Semester field is required.</span>");
        return;
    }
    try {
        auto enrollment = m_enrollmentService->enrollStudent(
            static_cast<int64_t>(m_enrollStudentId->value()),
            static_cast<int64_t>(m_enrollCourseId->value()),
            m_enrollSemester->text().trimmed().toStdString(),
            m_enrollYear->value()
        );
        m_enrollResultLabel->setText(
            QString("<span style='color:#27ae60;'>✓ Enrolled successfully. "
                    "Enrollment ID: <b>%1</b>, Status: <b>%2</b></span>")
                .arg(enrollment.id())
                .arg(enrollmentStatusString(enrollment.status()))
        );
    } catch (const std::exception& e) {
        m_enrollResultLabel->setText(
            QString("<span style='color:#e74c3c;'>✗ %1</span>")
                .arg(QString::fromStdString(e.what()))
        );
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab 2 slots
// ─────────────────────────────────────────────────────────────────────────────
void EnrollmentsPage::onLoadStudentEnrollments()
{
    if (!m_enrollmentService) return;
    try {
        int64_t sid = static_cast<int64_t>(m_studentIdInput->value());
        auto enrollments = m_enrollmentService->getStudentEnrollments(sid);
        // Tab 2: show Course ID but not Student ID (it's the same for all rows)
        m_studentEnrollTable->setColumnCount(7);
        m_studentEnrollTable->setHorizontalHeaderLabels(
            {"ID", "Course ID", "Semester", "Year", "Status", "Grade", "Letter"});

        m_studentEnrollTable->setRowCount(0);
        for (const auto& e : enrollments) {
            int row = m_studentEnrollTable->rowCount();
            m_studentEnrollTable->insertRow(row);
            int col = 0;
            auto setCell = [&](const QString& text, int align = Qt::AlignLeft | Qt::AlignVCenter) {
                auto* item = new QTableWidgetItem(text);
                item->setTextAlignment(align);
                m_studentEnrollTable->setItem(row, col++, item);
            };
            setCell(QString::number(e.id()),         Qt::AlignCenter);
            setCell(QString::number(e.courseId()),   Qt::AlignCenter);
            setCell(QString::fromStdString(e.semester()));
            setCell(QString::number(e.year()),       Qt::AlignCenter);
            setCell(enrollmentStatusString(e.status()), Qt::AlignCenter);
            setCell(e.gradePosted()
                ? QString::number(e.grade(), 'f', 1) : "—", Qt::AlignCenter);
            setCell(e.letterGrade().empty() ? "—"
                : QString::fromStdString(e.letterGrade()), Qt::AlignCenter);
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

void EnrollmentsPage::onApproveEnrollment()
{
    int64_t eid = selectedEnrollmentId(m_studentEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->approveEnrollment(eid);
        QMessageBox::information(this, "Success", QString("Enrollment %1 approved.").arg(eid));
        onLoadStudentEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onDropEnrollment()
{
    int64_t eid = selectedEnrollmentId(m_studentEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->dropCourse(eid);
        QMessageBox::information(this, "Success", QString("Enrollment %1 dropped.").arg(eid));
        onLoadStudentEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onWithdrawEnrollment()
{
    int64_t eid = selectedEnrollmentId(m_studentEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->withdrawFromCourse(eid);
        QMessageBox::information(this, "Success", QString("Withdrawn from enrollment %1.").arg(eid));
        onLoadStudentEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onCompleteEnrollment()
{
    int64_t eid = selectedEnrollmentId(m_studentEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->completeEnrollment(eid);
        QMessageBox::information(this, "Success", QString("Enrollment %1 marked complete.").arg(eid));
        onLoadStudentEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onPostGradeStudent()
{
    showPostGradeDialog(selectedEnrollmentId(m_studentEnrollTable));
    onLoadStudentEnrollments();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab 3 slots
// ─────────────────────────────────────────────────────────────────────────────
void EnrollmentsPage::onLoadCourseEnrollments()
{
    if (!m_enrollmentService) return;
    try {
        int64_t cid = static_cast<int64_t>(m_courseIdInput->value());
        auto enrollments = m_enrollmentService->getCourseEnrollments(cid);

        m_courseEnrollTable->setColumnCount(7);
        m_courseEnrollTable->setHorizontalHeaderLabels(
            {"ID", "Student ID", "Semester", "Year", "Status", "Grade", "Letter"});

        m_courseEnrollTable->setRowCount(0);
        for (const auto& e : enrollments) {
            int row = m_courseEnrollTable->rowCount();
            m_courseEnrollTable->insertRow(row);
            int col = 0;
            auto setCell = [&](const QString& text, int align = Qt::AlignLeft | Qt::AlignVCenter) {
                auto* item = new QTableWidgetItem(text);
                item->setTextAlignment(align);
                m_courseEnrollTable->setItem(row, col++, item);
            };
            setCell(QString::number(e.id()),         Qt::AlignCenter);
            setCell(QString::number(e.studentId()),  Qt::AlignCenter);
            setCell(QString::fromStdString(e.semester()));
            setCell(QString::number(e.year()),       Qt::AlignCenter);
            setCell(enrollmentStatusString(e.status()), Qt::AlignCenter);
            setCell(e.gradePosted()
                ? QString::number(e.grade(), 'f', 1) : "—", Qt::AlignCenter);
            setCell(e.letterGrade().empty() ? "—"
                : QString::fromStdString(e.letterGrade()), Qt::AlignCenter);
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

void EnrollmentsPage::onApproveEnrollmentCourse()
{
    int64_t eid = selectedEnrollmentId(m_courseEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->approveEnrollment(eid);
        QMessageBox::information(this, "Success", QString("Enrollment %1 approved.").arg(eid));
        onLoadCourseEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onDropEnrollmentCourse()
{
    int64_t eid = selectedEnrollmentId(m_courseEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->dropCourse(eid);
        QMessageBox::information(this, "Success", QString("Enrollment %1 dropped.").arg(eid));
        onLoadCourseEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onWithdrawEnrollmentCourse()
{
    int64_t eid = selectedEnrollmentId(m_courseEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->withdrawFromCourse(eid);
        QMessageBox::information(this, "Success", QString("Withdrawn from enrollment %1.").arg(eid));
        onLoadCourseEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onCompleteEnrollmentCourse()
{
    int64_t eid = selectedEnrollmentId(m_courseEnrollTable);
    if (eid < 0) { QMessageBox::information(this, "No Selection", "Select an enrollment first."); return; }
    try {
        m_enrollmentService->completeEnrollment(eid);
        QMessageBox::information(this, "Success", QString("Enrollment %1 marked complete.").arg(eid));
        onLoadCourseEnrollments();
    } catch (const std::exception& e) { QMessageBox::critical(this, "Error", e.what()); }
}

void EnrollmentsPage::onPostGradeCourse()
{
    showPostGradeDialog(selectedEnrollmentId(m_courseEnrollTable));
    onLoadCourseEnrollments();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab 4 slots
// ─────────────────────────────────────────────────────────────────────────────
void EnrollmentsPage::onRefreshPendingGrades()
{
    if (!m_enrollmentService) return;
    try {
        auto enrollments = m_enrollmentService->getPendingGrades();

        m_pendingGradesTable->setColumnCount(8);
        m_pendingGradesTable->setHorizontalHeaderLabels(
            {"ID", "Student ID", "Course ID", "Semester", "Year", "Status", "Grade", "Letter"});

        m_pendingGradesTable->setRowCount(0);
        for (const auto& e : enrollments) {
            int row = m_pendingGradesTable->rowCount();
            m_pendingGradesTable->insertRow(row);
            int col = 0;
            auto setCell = [&](const QString& text, int align = Qt::AlignLeft | Qt::AlignVCenter) {
                auto* item = new QTableWidgetItem(text);
                item->setTextAlignment(align);
                m_pendingGradesTable->setItem(row, col++, item);
            };
            setCell(QString::number(e.id()),         Qt::AlignCenter);
            setCell(QString::number(e.studentId()),  Qt::AlignCenter);
            setCell(QString::number(e.courseId()),   Qt::AlignCenter);
            setCell(QString::fromStdString(e.semester()));
            setCell(QString::number(e.year()),       Qt::AlignCenter);
            setCell(enrollmentStatusString(e.status()), Qt::AlignCenter);
            setCell(e.gradePosted()
                ? QString::number(e.grade(), 'f', 1) : "—", Qt::AlignCenter);
            setCell(e.letterGrade().empty() ? "—"
                : QString::fromStdString(e.letterGrade()), Qt::AlignCenter);
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

void EnrollmentsPage::onPostGradePending()
{
    showPostGradeDialog(selectedEnrollmentId(m_pendingGradesTable));
    onRefreshPendingGrades();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab 5 slots
// ─────────────────────────────────────────────────────────────────────────────
void EnrollmentsPage::onLoadTranscript()
{
    if (!m_enrollmentService) return;
    int64_t sid = static_cast<int64_t>(m_transcriptStudentId->value());

    // GPA
    try {
        double gpa = m_enrollmentService->getStudentGPA(sid);
        m_gpaLabel->setText(QString::number(gpa, 'f', 2));
        // Color-code GPA
        if (gpa >= 3.5)
            m_gpaLabel->setStyleSheet("color: #27ae60;");
        else if (gpa >= 2.0)
            m_gpaLabel->setStyleSheet("color: #3498db;");
        else
            m_gpaLabel->setStyleSheet("color: #e74c3c;");
    } catch (const std::exception& e) {
        m_gpaLabel->setText("—");
        QMessageBox::critical(this, "GPA Error", e.what());
        return;
    }

    // Transcript
    try {
        std::string transcriptJson = m_enrollmentService->generateTranscript(sid);

        // Parse JSON and format nicely
        QString formatted;
        try {
            auto j = json::parse(transcriptJson);

            std::ostringstream out;
            out << "═══════════════════════════════════════════\n";
            out << "               ACADEMIC TRANSCRIPT\n";
            out << "═══════════════════════════════════════════\n";

            if (j.contains("studentId"))
                out << "Student ID : " << j["studentId"] << "\n";
            if (j.contains("studentName"))
                out << "Name       : " << j["studentName"].get<std::string>() << "\n";
            if (j.contains("generatedAt"))
                out << "Generated  : " << j["generatedAt"].get<std::string>() << "\n";
            if (j.contains("gpa"))
                out << "Cumulative GPA : " << j["gpa"] << "\n";
            if (j.contains("totalCredits"))
                out << "Total Credits  : " << j["totalCredits"] << "\n";

            out << "\n─── Enrollments ──────────────────────────────\n";

            if (j.contains("enrollments") && j["enrollments"].is_array()) {
                for (const auto& enr : j["enrollments"]) {
                    out << "\n";
                    if (enr.contains("courseCode"))
                        out << "  Course   : " << enr["courseCode"].get<std::string>() << "\n";
                    if (enr.contains("courseTitle"))
                        out << "  Title    : " << enr["courseTitle"].get<std::string>() << "\n";
                    if (enr.contains("semester"))
                        out << "  Semester : " << enr["semester"].get<std::string>() << "\n";
                    if (enr.contains("year"))
                        out << "  Year     : " << enr["year"] << "\n";
                    if (enr.contains("grade"))
                        out << "  Grade    : " << enr["grade"] << "\n";
                    if (enr.contains("letterGrade"))
                        out << "  Letter   : " << enr["letterGrade"].get<std::string>() << "\n";
                    if (enr.contains("status"))
                        out << "  Status   : " << enr["status"].get<std::string>() << "\n";
                    out << "  ──────────────────────────────────────\n";
                }
            }

            out << "\n═══════════════════════════════════════════\n";
            formatted = QString::fromStdString(out.str());
        } catch (...) {
            // Fall back to raw JSON if parsing fails
            formatted = QString::fromStdString(transcriptJson);
        }

        m_transcriptText->setPlainText(formatted);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Transcript Error", e.what());
    }
}

} // namespace sms::app::gui
