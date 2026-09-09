#include "DashboardPage.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QColor>
#include <QMessageBox>
#include <QSizePolicy>

namespace sms::app::gui {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
DashboardPage::DashboardPage(
    sms::app::authentication::services::IAuthenticationService* authService,
    sms::app::students::services::IStudentService*               studentService,
    sms::app::courses::repositories::ICourseRepository*          courseRepo,
    sms::app::enrollments::services::IEnrollmentService*         enrollmentService,
    const QString& currentUserFirstName,
    QWidget* parent
)
    : QWidget(parent)
    , m_authService(authService)
    , m_studentService(studentService)
    , m_courseRepo(courseRepo)
    , m_enrollmentService(enrollmentService)
{
    setObjectName("DashboardPage");

    // ── Outer layout ─────────────────────────────────────────────────────────
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(24, 24, 24, 24);
    outerLayout->setSpacing(20);

    // ── Header row ───────────────────────────────────────────────────────────
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto* headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    m_welcomeLabel = new QLabel(QString("Welcome, %1!").arg(currentUserFirstName), this);
    QFont welcomeFont;
    welcomeFont.setPointSize(22);
    welcomeFont.setBold(true);
    m_welcomeLabel->setFont(welcomeFont);
    m_welcomeLabel->setStyleSheet("color: #2c3e50;");

    auto* subtitleLabel = new QLabel("University of Excellence", this);
    QFont subtitleFont;
    subtitleFont.setPointSize(12);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color: #7f8c8d;");

    headerTextLayout->addWidget(m_welcomeLabel);
    headerTextLayout->addWidget(subtitleLabel);

    auto* refreshBtn = new QPushButton("⟳  Refresh", this);
    refreshBtn->setFixedHeight(38);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3498db;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 0 20px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #2980b9; }"
        "QPushButton:pressed { background-color: #2471a3; }"
    );

    headerLayout->addLayout(headerTextLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshBtn, 0, Qt::AlignVCenter);

    // ── Card grid ─────────────────────────────────────────────────────────────
    auto* cardGrid = new QGridLayout();
    cardGrid->setSpacing(20);
    cardGrid->setContentsMargins(0, 0, 0, 0);

    // Card accent colors
    const QString blueAccent   = "#3498db";
    const QString greenAccent  = "#2ecc71";
    const QString purpleAccent = "#9b59b6";
    const QString orangeAccent = "#e67e22";

    QFrame* card0 = makeCard(blueAccent,   "👥", "Total Students",  m_totalStudentsCount);
    QFrame* card1 = makeCard(greenAccent,  "✅", "Active Students", m_activeStudentsCount);
    QFrame* card2 = makeCard(purpleAccent, "📚", "Total Courses",   m_totalCoursesCount);
    QFrame* card3 = makeCard(orangeAccent, "⏳", "Pending Grades",  m_pendingGradesCount);

    cardGrid->addWidget(card0, 0, 0);
    cardGrid->addWidget(card1, 0, 1);
    cardGrid->addWidget(card2, 1, 0);
    cardGrid->addWidget(card3, 1, 1);

    cardGrid->setColumnStretch(0, 1);
    cardGrid->setColumnStretch(1, 1);
    cardGrid->setRowStretch(0, 1);
    cardGrid->setRowStretch(1, 1);

    // ── Assemble ─────────────────────────────────────────────────────────────
    outerLayout->addLayout(headerLayout);
    outerLayout->addLayout(cardGrid, 1);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(refreshBtn, &QPushButton::clicked, this, &DashboardPage::refresh);

    // Initial data load
    refresh();
}

// ─────────────────────────────────────────────────────────────────────────────
// refresh()
// ─────────────────────────────────────────────────────────────────────────────
void DashboardPage::refresh()
{
    // Total students
    try {
        int64_t total = m_studentService ? m_studentService->getTotalStudentCount() : 0;
        m_totalStudentsCount->setText(QString::number(total));
    } catch (...) {
        m_totalStudentsCount->setText("0");
    }

    // Active students
    try {
        int64_t active = m_studentService ? m_studentService->getActiveStudentCount() : 0;
        m_activeStudentsCount->setText(QString::number(active));
    } catch (...) {
        m_activeStudentsCount->setText("0");
    }

    // Total courses
    try {
        int total = 0;
        if (m_courseRepo) {
            auto courses = m_courseRepo->readAll();
            total = static_cast<int>(courses.size());
        }
        m_totalCoursesCount->setText(QString::number(total));
    } catch (...) {
        m_totalCoursesCount->setText("0");
    }

    // Pending grades
    try {
        int pending = 0;
        if (m_enrollmentService) {
            auto list = m_enrollmentService->getPendingGrades();
            pending = static_cast<int>(list.size());
        }
        m_pendingGradesCount->setText(QString::number(pending));
    } catch (...) {
        m_pendingGradesCount->setText("0");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// makeCard()
// ─────────────────────────────────────────────────────────────────────────────
QFrame* DashboardPage::makeCard(const QString& accentColor,
                                 const QString& icon,
                                 const QString& label,
                                 QLabel*&        countLabelOut)
{
    // Outer frame (card)
    auto* card = new QFrame(this);
    card->setObjectName("StatCard");
    card->setFrameShape(QFrame::StyledPanel);
    card->setMinimumHeight(160);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    card->setStyleSheet(
        "QFrame#StatCard {"
        "  background-color: white;"
        "  border-radius: 12px;"
        "  border: none;"
        "}"
    );

    // Drop shadow
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(15);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 32)); // #00000020
    card->setGraphicsEffect(shadow);

    // Layout inside card
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // Accent top border (4 px colored QFrame)
    auto* accentBar = new QFrame(card);
    accentBar->setFixedHeight(4);
    accentBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    accentBar->setStyleSheet(
        QString("QFrame { background-color: %1; border-top-left-radius: 12px; "
                "border-top-right-radius: 12px; border: none; }").arg(accentColor)
    );

    // Content area
    auto* content = new QWidget(card);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 18, 24, 18);
    contentLayout->setSpacing(8);
    contentLayout->setAlignment(Qt::AlignCenter);

    // Icon + label row
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    auto* iconLabel = new QLabel(icon, content);
    QFont iconFont;
    iconFont.setPointSize(22);
    iconLabel->setFont(iconFont);

    auto* nameLabel = new QLabel(label, content);
    QFont nameFont;
    nameFont.setPointSize(13);
    nameLabel->setFont(nameFont);
    nameLabel->setStyleSheet("color: #7f8c8d;");
    nameLabel->setWordWrap(true);

    topRow->addWidget(iconLabel, 0, Qt::AlignVCenter);
    topRow->addWidget(nameLabel, 1, Qt::AlignVCenter);

    // Count label
    auto* countLabel = new QLabel("—", content);
    QFont countFont;
    countFont.setPointSize(36);
    countFont.setBold(true);
    countLabel->setFont(countFont);
    countLabel->setStyleSheet("color: #2c3e50;");
    countLabel->setAlignment(Qt::AlignCenter);

    contentLayout->addLayout(topRow);
    contentLayout->addWidget(countLabel, 0, Qt::AlignCenter);

    cardLayout->addWidget(accentBar);
    cardLayout->addWidget(content, 1);

    countLabelOut = countLabel;
    return card;
}

} // namespace sms::app::gui
