#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <memory>

#include "app/authentication/services/IAuthenticationService.hpp"

namespace sms::app::gui {

using sms::app::authentication::services::IAuthenticationService;
using sms::app::authentication::services::LoginResult;

/**
 * @brief Modal login dialog
 *
 * Presents email + password fields, delegates to IAuthenticationService,
 * and stores the resulting LoginResult for the caller to retrieve.
 */
class LoginDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct the login dialog
     * @param auth   Authentication service used to validate credentials
     * @param parent Optional Qt parent
     */
    explicit LoginDialog(
        std::shared_ptr<IAuthenticationService> auth,
        QWidget* parent = nullptr
    );

    ~LoginDialog() override = default;

    /**
     * @brief Get the login result after a successful login
     * @return LoginResult containing the User and session token
     */
    [[nodiscard]] const LoginResult& loginResult() const { return m_loginResult; }

private slots:
    void onLoginClicked();

private:
    std::shared_ptr<IAuthenticationService> m_auth;
    LoginResult                             m_loginResult;

    QLineEdit*   m_emailField{nullptr};
    QLineEdit*   m_passwordField{nullptr};
    QPushButton* m_loginBtn{nullptr};
    QLabel*      m_errorLabel{nullptr};
};

} // namespace sms::app::gui
