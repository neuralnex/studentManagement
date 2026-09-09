#include "LoginDialog.hpp"

#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QString>

#include <spdlog/spdlog.h>

namespace sms::app::gui {

// ─── Button stylesheet ───────────────────────────────────────────────────────

static constexpr const char* LOGIN_BTN_STYLE = R"(
QPushButton {
    background-color: #3498db;
    color: white;
    border: none;
    border-radius: 5px;
    font-size: 14px;
    font-weight: bold;
}
QPushButton:hover {
    background-color: #2980b9;
}
QPushButton:pressed {
    background-color: #21618c;
}
)";

// ─── Constructor ─────────────────────────────────────────────────────────────

LoginDialog::LoginDialog(
    std::shared_ptr<IAuthenticationService> auth,
    QWidget* parent
)
    : QDialog(parent)
    , m_auth(std::move(auth))
{
    setWindowTitle(QStringLiteral("Student Management System \u2014 Login"));
    setFixedSize(420, 500);
    setStyleSheet("background-color: white;");
    // Prevent resize handle from appearing on some platforms
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ── Root layout ─────────────────────────────────────────────────────────
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 30, 40, 30);
    root->setSpacing(0);

    // ── Logo area ────────────────────────────────────────────────────────────
    auto* logoLabel = new QLabel(QStringLiteral("SMS"), this);
    QFont logoFont;
    logoFont.setPointSize(48);
    logoFont.setBold(true);
    logoLabel->setFont(logoFont);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("color: #3498db; background: transparent;");
    root->addWidget(logoLabel);

    auto* subtitleLabel = new QLabel(
        QStringLiteral("Student Management System"), this
    );
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "color: #7f8c8d; font-size: 14px; background: transparent;"
    );
    root->addWidget(subtitleLabel);

    root->addSpacing(16);

    // ── Divider ──────────────────────────────────────────────────────────────
    auto* divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);
    divider->setStyleSheet("color: #dcdde1;");
    root->addWidget(divider);

    root->addSpacing(24);

    // ── Email field ──────────────────────────────────────────────────────────
    auto* emailLabel = new QLabel(QStringLiteral("Email Address"), this);
    emailLabel->setStyleSheet(
        "font-weight: bold; color: #2c3e50; font-size: 13px; background: transparent;"
    );
    root->addWidget(emailLabel);

    root->addSpacing(6);

    m_emailField = new QLineEdit(this);
    m_emailField->setPlaceholderText(QStringLiteral("Enter your email"));
    m_emailField->setFixedHeight(40);
    root->addWidget(m_emailField);

    root->addSpacing(16);

    // ── Password field ───────────────────────────────────────────────────────
    auto* passwordLabel = new QLabel(QStringLiteral("Password"), this);
    passwordLabel->setStyleSheet(
        "font-weight: bold; color: #2c3e50; font-size: 13px; background: transparent;"
    );
    root->addWidget(passwordLabel);

    root->addSpacing(6);

    m_passwordField = new QLineEdit(this);
    m_passwordField->setPlaceholderText(QStringLiteral("Enter your password"));
    m_passwordField->setEchoMode(QLineEdit::Password);
    m_passwordField->setFixedHeight(40);
    root->addWidget(m_passwordField);

    // Pressing Enter in the password field triggers login
    connect(
        m_passwordField, &QLineEdit::returnPressed,
        this,            &LoginDialog::onLoginClicked
    );

    root->addSpacing(24);

    // ── Login button ─────────────────────────────────────────────────────────
    m_loginBtn = new QPushButton(QStringLiteral("Login"), this);
    m_loginBtn->setFixedHeight(40);
    m_loginBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_loginBtn->setStyleSheet(LOGIN_BTN_STYLE);
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    root->addWidget(m_loginBtn);

    root->addSpacing(12);

    // ── Error label ───────────────────────────────────────────────────────────
    m_errorLabel = new QLabel(this);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet(
        "color: #e74c3c; font-size: 12px; background: transparent;"
    );
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    root->addStretch();
}

// ─── Slot: onLoginClicked ────────────────────────────────────────────────────

void LoginDialog::onLoginClicked() {
    const std::string email    = m_emailField->text().trimmed().toStdString();
    const std::string password = m_passwordField->text().toStdString();

    if (email.empty() || password.empty()) {
        m_errorLabel->setText(
            QStringLiteral("Please enter your email and password.")
        );
        m_errorLabel->show();
        return;
    }

    m_loginBtn->setEnabled(false);
    m_loginBtn->setText(QStringLiteral("Logging in\u2026"));
    m_errorLabel->hide();

    try {
        m_loginResult = m_auth->login(email, password);
        spdlog::info("Login successful for '{}'", email);
        accept(); // closes the dialog with QDialog::Accepted
    } catch (const std::exception& ex) {
        spdlog::warn("Login failed for '{}': {}", email, ex.what());
        m_errorLabel->setText(QString::fromStdString(ex.what()));
        m_errorLabel->show();
        m_passwordField->clear();
        m_passwordField->setFocus();
    }

    m_loginBtn->setEnabled(true);
    m_loginBtn->setText(QStringLiteral("Login"));
}

} // namespace sms::app::gui
