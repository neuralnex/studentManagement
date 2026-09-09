#include "StudentsPage.hpp"

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

using namespace sms::app::students::domain;

namespace sms::app::gui {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
QString StudentsPage::studentStatusString(StudentStatus s)
{
    switch (s) {
        case StudentStatus::ACTIVE:    return "Active";
        case StudentStatus::SUSPENDED: return "Suspended";
        case StudentStatus::GRADUATED: return "Graduated";
        case StudentStatus::INACTIVE:  return "Inactive";
        case StudentStatus::DELETED:   return "Deleted";
    }
    return "Unknown";
}

QString StudentsPage::academicStandingString(AcademicStanding a)
{
    switch (a) {
        case AcademicStanding::GOOD:      return "Good";
        case AcademicStanding::PROBATION: return "Probation";
        case AcademicStanding::WARNING:   return "Warning";
        case AcademicStanding::SUSPENDED: return "Suspended";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
StudentsPage::StudentsPage(
    sms::app::students::services::IStudentService* studentService,
    QWidget* parent
)
    : QWidget(parent)
    , m_studentService(studentService)
{
    setObjectName("StudentsPage");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    // ── Top bar ──────────────────────────────────────────────────────────────
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(10);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Search by name, matric, or email...");
    m_searchBar->setMinimumHeight(36);
    m_searchBar->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #bdc3c7;"
        "  border-radius: 6px;"
        "  padding: 0 10px;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus { border-color: #3498db; }"
    );

    auto* addBtn = new QPushButton("+ Add Student", this);
    addBtn->setFixedHeight(36);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3498db; color: white;"
        "  border: none; border-radius: 6px;"
        "  padding: 0 18px; font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #2980b9; }"
        "QPushButton:pressed { background-color: #2471a3; }"
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
    m_table = new QTableWidget(0, 7, this);
    m_table->setHorizontalHeaderLabels(
        {"ID", "Matric No", "Full Name", "Email", "Phone", "Status", "Standing"});
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
        "  background-color: #f5f6fa;"
        "  font-weight: bold; color: #2c3e50;"
        "  border: none; border-bottom: 2px solid #e0e0e0;"
        "  padding: 8px;"
        "}"
    );

    // ── Action bar ────────────────────────────────────────────────────────────
    auto* actionBar = new QHBoxLayout();
    actionBar->setSpacing(10);

    auto makeActionBtn = [&](const QString& text, const QString& color) -> QPushButton* {
        auto* btn = new QPushButton(text, this);
        btn->setFixedHeight(34);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QString("QPushButton { background-color: %1; color: white;"
                    " border: none; border-radius: 5px; padding: 0 14px; font-size: 13px; }"
                    "QPushButton:hover { opacity: 0.9; }"
                    "QPushButton:disabled { background-color: #bdc3c7; }").arg(color)
        );
        return btn;
    };

    m_viewBtn        = makeActionBtn("View",              "#3498db");
    m_editStatusBtn  = makeActionBtn("Edit Status",       "#e67e22");
    m_emergencyBtn   = makeActionBtn("Emergency Contact", "#9b59b6");

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #7f8c8d; font-size: 12px;");

    actionBar->addWidget(m_viewBtn);
    actionBar->addWidget(m_editStatusBtn);
    actionBar->addWidget(m_emergencyBtn);
    actionBar->addStretch();
    actionBar->addWidget(m_statusLabel);

    // ── Assemble ──────────────────────────────────────────────────────────────
    layout->addLayout(topBar);
    layout->addWidget(m_table, 1);
    layout->addLayout(actionBar);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_searchBar,    &QLineEdit::textChanged, this, &StudentsPage::onSearchTextChanged);
    connect(addBtn,         &QPushButton::clicked,   this, &StudentsPage::onAddStudent);
    connect(refreshBtn,     &QPushButton::clicked,   this, &StudentsPage::refresh);
    connect(m_viewBtn,      &QPushButton::clicked,   this, &StudentsPage::onViewStudent);
    connect(m_editStatusBtn,&QPushButton::clicked,   this, &StudentsPage::onEditStatus);
    connect(m_emergencyBtn, &QPushButton::clicked,   this, &StudentsPage::onEmergencyContact);

    refresh();
}

// ─────────────────────────────────────────────────────────────────────────────
// populateTable
// ─────────────────────────────────────────────────────────────────────────────
void StudentsPage::populateTable(const std::vector<Student>& students)
{
    m_table->setRowCount(0);
    for (const auto& s : students) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto setCell = [&](int col, const QString& text, int align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            m_table->setItem(row, col, item);
        };

        setCell(0, QString::number(s.id()),                Qt::AlignCenter);
        setCell(1, QString::fromStdString(s.matricNumber()));
        setCell(2, QString::fromStdString(s.getFullName()));
        setCell(3, QString::fromStdString(s.email()));
        setCell(4, QString::fromStdString(s.phoneNumber()));
        setCell(5, studentStatusString(s.status()),        Qt::AlignCenter);
        setCell(6, academicStandingString(s.academicStanding()), Qt::AlignCenter);

        // Color-code status
        auto* statusItem = m_table->item(row, 5);
        if (statusItem) {
            switch (s.status()) {
                case StudentStatus::ACTIVE:
                    statusItem->setForeground(QColor("#27ae60")); break;
                case StudentStatus::SUSPENDED:
                    statusItem->setForeground(QColor("#e74c3c")); break;
                case StudentStatus::GRADUATED:
                    statusItem->setForeground(QColor("#3498db")); break;
                default:
                    statusItem->setForeground(QColor("#7f8c8d")); break;
            }
        }
    }
    m_statusLabel->setText(QString("%1 student(s) shown").arg(students.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
// refresh
// ─────────────────────────────────────────────────────────────────────────────
void StudentsPage::refresh()
{
    try {
        if (!m_studentService) return;
        auto students = m_studentService->getActiveStudents(0, 500);
        populateTable(students);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// onSearchTextChanged
// ─────────────────────────────────────────────────────────────────────────────
void StudentsPage::onSearchTextChanged(const QString& text)
{
    if (!m_studentService) return;
    try {
        if (text.length() >= 3) {
            auto results = m_studentService->searchStudents(text.toStdString());
            populateTable(results);
        } else if (text.isEmpty()) {
            refresh();
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Search Error", e.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// selectedStudentId
// ─────────────────────────────────────────────────────────────────────────────
int64_t StudentsPage::selectedStudentId() const
{
    int row = m_table->currentRow();
    if (row < 0) return -1;
    auto* item = m_table->item(row, 0);
    if (!item) return -1;
    return item->text().toLongLong();
}

// ─────────────────────────────────────────────────────────────────────────────
// onAddStudent  →  StudentFormDialog
// ─────────────────────────────────────────────────────────────────────────────
void StudentsPage::onAddStudent()
{
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Add Student");
    dlg->setModal(true);
    dlg->setMinimumWidth(420);

    auto* form = new QFormLayout(dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    auto* userIdSpin  = new QSpinBox(dlg);
    userIdSpin->setRange(1, INT_MAX);

    auto* matricEdit  = new QLineEdit(dlg);
    auto* firstEdit   = new QLineEdit(dlg);
    auto* lastEdit    = new QLineEdit(dlg);
    auto* emailEdit   = new QLineEdit(dlg);
    auto* phoneEdit   = new QLineEdit(dlg);

    form->addRow("User ID:",      userIdSpin);
    form->addRow("Matric No:",    matricEdit);
    form->addRow("First Name:",   firstEdit);
    form->addRow("Last Name:",    lastEdit);
    form->addRow("Email:",        emailEdit);
    form->addRow("Phone:",        phoneEdit);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    buttons->button(QDialogButtonBox::Ok)->setText("Register");
    buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none;"
        " border-radius: 5px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dlg, [&, dlg]() {
        if (matricEdit->text().trimmed().isEmpty() ||
            firstEdit->text().trimmed().isEmpty()  ||
            lastEdit->text().trimmed().isEmpty()   ||
            emailEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(dlg, "Validation", "Please fill in all required fields.");
            return;
        }
        try {
            m_studentService->registerStudent(
                static_cast<int64_t>(userIdSpin->value()),
                matricEdit->text().trimmed().toStdString(),
                firstEdit->text().trimmed().toStdString(),
                lastEdit->text().trimmed().toStdString(),
                emailEdit->text().trimmed().toStdString(),
                phoneEdit->text().trimmed().toStdString()
            );
            QMessageBox::information(dlg, "Success", "Student registered successfully.");
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
// onViewStudent
// ─────────────────────────────────────────────────────────────────────────────
void StudentsPage::onViewStudent()
{
    int64_t sid = selectedStudentId();
    if (sid < 0) {
        QMessageBox::information(this, "No Selection", "Please select a student first.");
        return;
    }
    try {
        auto opt = m_studentService->getStudent(sid);
        if (!opt) {
            QMessageBox::warning(this, "Not Found", "Student not found.");
            return;
        }
        const auto& s = *opt;

        auto* dlg = new QDialog(this);
        dlg->setWindowTitle(QString("Student Details — %1")
                            .arg(QString::fromStdString(s.getFullName())));
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

        addRow("ID",               QString::number(s.id()));
        addRow("User ID",          QString::number(s.userId()));
        addRow("Matric No",        QString::fromStdString(s.matricNumber()));
        addRow("First Name",       QString::fromStdString(s.firstName()));
        addRow("Last Name",        QString::fromStdString(s.lastName()));
        addRow("Email",            QString::fromStdString(s.email()));
        addRow("Phone",            QString::fromStdString(s.phoneNumber()));
        addRow("Status",           studentStatusString(s.status()));
        addRow("Academic Standing",academicStandingString(s.academicStanding()));
        addRow("Emergency Contact",QString::fromStdString(s.emergencyContactName()));
        addRow("Emergency Phone",  QString::fromStdString(s.emergencyContactPhone()));
        addRow("Guardian Name",    QString::fromStdString(s.guardianName()));
        addRow("Guardian Phone",   QString::fromStdString(s.guardianPhone()));
        addRow("Guardian Email",   QString::fromStdString(s.guardianEmail()));

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
// onEditStatus
// ─────────────────────────────────────────────────────────────────────────────
void StudentsPage::onEditStatus()
{
    int64_t sid = selectedStudentId();
    if (sid < 0) {
        QMessageBox::information(this, "No Selection", "Please select a student first.");
        return;
    }

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Edit Student Status");
    dlg->setModal(true);
    dlg->setMinimumWidth(400);

    auto* form = new QFormLayout(dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    auto* actionCombo = new QComboBox(dlg);
    actionCombo->addItems({"Suspend", "Reactivate", "Graduate"});

    auto* reasonEdit = new QLineEdit(dlg);
    reasonEdit->setPlaceholderText("Reason (required for suspend)");

    form->addRow("Action:",        actionCombo);
    form->addRow("Reason:",        reasonEdit);

    // Show/hide reason based on action
    connect(actionCombo, &QComboBox::currentTextChanged, dlg,
            [reasonEdit](const QString& text) {
                reasonEdit->setVisible(text == "Suspend");
            });
    reasonEdit->setVisible(true);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
        "QPushButton { background-color: #e67e22; color: white; border: none;"
        " border-radius: 5px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #ca6f1e; }"
    );
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dlg, [&, dlg, actionCombo, reasonEdit]() {
        try {
            const QString action = actionCombo->currentText();
            if (action == "Suspend") {
                m_studentService->suspendStudent(sid, reasonEdit->text().toStdString());
            } else if (action == "Reactivate") {
                m_studentService->reactivateStudent(sid);
            } else if (action == "Graduate") {
                m_studentService->graduateStudent(sid);
            }
            QMessageBox::information(dlg, "Success",
                QString("Student status updated to '%1'.").arg(action));
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
// onEmergencyContact
// ─────────────────────────────────────────────────────────────────────────────
void StudentsPage::onEmergencyContact()
{
    int64_t sid = selectedStudentId();
    if (sid < 0) {
        QMessageBox::information(this, "No Selection", "Please select a student first.");
        return;
    }

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Set Emergency Contact");
    dlg->setModal(true);
    dlg->setMinimumWidth(400);

    auto* form = new QFormLayout(dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    auto* nameEdit  = new QLineEdit(dlg);
    nameEdit->setPlaceholderText("Contact full name");
    auto* phoneEdit = new QLineEdit(dlg);
    phoneEdit->setPlaceholderText("+1 555 000 0000");

    form->addRow("Name:",  nameEdit);
    form->addRow("Phone:", phoneEdit);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, dlg);
    buttons->button(QDialogButtonBox::Save)->setStyleSheet(
        "QPushButton { background-color: #9b59b6; color: white; border: none;"
        " border-radius: 5px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #8e44ad; }"
    );
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dlg, [&, dlg, nameEdit, phoneEdit]() {
        if (nameEdit->text().trimmed().isEmpty() || phoneEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(dlg, "Validation", "Name and phone are required.");
            return;
        }
        try {
            m_studentService->setEmergencyContact(
                sid,
                nameEdit->text().trimmed().toStdString(),
                phoneEdit->text().trimmed().toStdString()
            );
            QMessageBox::information(dlg, "Success", "Emergency contact saved.");
            dlg->accept();
        } catch (const std::exception& e) {
            QMessageBox::critical(dlg, "Error", e.what());
        }
    });
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    dlg->exec();
    dlg->deleteLater();
}

} // namespace sms::app::gui
