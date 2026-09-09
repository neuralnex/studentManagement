#pragma once

#include <ncurses.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>


#include "app/authentication/services/IAuthenticationService.hpp"

#include "app/students/services/IStudentService.hpp"
#include "app/courses/repositories/ICourseRepository.hpp"
#include "app/enrollments/services/IEnrollmentService.hpp"

namespace sms::app::ui {

using namespace sms::app::authentication::services;
using namespace sms::app::authentication::domain;
using namespace sms::app::students::services;
using namespace sms::app::students::domain;
using namespace sms::app::courses::repositories;
using namespace sms::app::courses::domain;
using namespace sms::app::enrollments::services;
using namespace sms::app::enrollments::domain;

// ─── Color pair constants ──────────────────────────────────────────────────
static constexpr int CP_HEADER    = 1;  ///< White on blue  (header/status bars)
static constexpr int CP_SELECTED  = 2;  ///< Black on cyan  (selected item)
static constexpr int CP_ERROR     = 3;  ///< Red on black   (errors)
static constexpr int CP_SUCCESS   = 4;  ///< Green on black (success)
static constexpr int CP_WARNING   = 5;  ///< Yellow on black (warnings)

/**
 * @brief ncurses Text User Interface for the Student Management System.
 *
 * Owns the terminal lifecycle (initscr … endwin).  Call run() once.
 */
class TUI {
public:
    /**
     * @brief Construct TUI with all required services.
     * @param authService  Authentication / session service
     * @param studentService  Student CRUD service
     * @param courseRepo   Course repository (read + write)
     * @param enrollService  Enrollment / grade service
     */
    TUI(
        std::shared_ptr<IAuthenticationService> authService,
        std::shared_ptr<IStudentService>        studentService,
        std::shared_ptr<ICourseRepository>      courseRepo,
        std::shared_ptr<IEnrollmentService>     enrollService
    );

    ~TUI();

    // Non-copyable, non-movable (owns global ncurses state)
    TUI(const TUI&)            = delete;
    TUI& operator=(const TUI&) = delete;
    TUI(TUI&&)                 = delete;
    TUI& operator=(TUI&&)      = delete;

    /** @brief Start the TUI main loop.  Returns when the user exits. */
    void run();

private:
    // ── Services ────────────────────────────────────────────────────────────
    std::shared_ptr<IAuthenticationService> m_auth;
    std::shared_ptr<IStudentService>        m_students;
    std::shared_ptr<ICourseRepository>      m_courses;
    std::shared_ptr<IEnrollmentService>     m_enrollments;

    // ── Session state ────────────────────────────────────────────────────────
    std::optional<User>  m_currentUser;
    std::string          m_sessionToken;

    // ── Terminal dimensions (refreshed on KEY_RESIZE) ────────────────────────
    int m_rows{0};
    int m_cols{0};

    // ── ncurses lifecycle ────────────────────────────────────────────────────
    void initNcurses();
    void cleanupNcurses();
    void refreshDimensions();

    // ── Top-level screens ────────────────────────────────────────────────────
    bool showLogin();           ///< Returns true on successful login
    void showMainMenu();        ///< Post-login menu loop

    // ── Sub-menus ───────────────────────────────────────────────────────────
    void showDashboard();
    void showStudentsMenu();
    void showCoursesMenu();
    void showEnrollmentsMenu();
    void showMyProfile();       ///< Student self-service
    void showMyEnrollments();   ///< Student self-service
    void showMyTranscript();    ///< Student self-service
    void showMyGPA();           ///< Student self-service
    void showReportsMenu();     ///< Admin reports

    // ── Students sub-actions ────────────────────────────────────────────────
    void listAllStudents();
    void registerStudentForm();
    void searchStudentsView();
    void viewStudentById();
    void updateStudentStatus();
    void setEmergencyContactForm();
    void showStudentStatistics();

    // ── Courses sub-actions ─────────────────────────────────────────────────
    void listAllCourses();
    void addCourseForm();
    void searchCoursesView();
    void viewCourseDetails();
    void updateCourseForm();

    // ── Enrollment sub-actions ───────────────────────────────────────────────
    void enrollStudentForm();
    void listStudentEnrollments();
    void listCourseEnrollments();
    void approveEnrollmentForm();
    void dropWithdrawForm();
    void postGradeForm();
    void viewStudentGPA();
    void viewTranscript();
    void showPendingGrades();

    // ── Generic UI helpers ───────────────────────────────────────────────────

    /** Draw a centred, bold title inside a bordered window. */
    void drawHeader(WINDOW* win, const std::string& title);

    /** Overwrite the bottom status line with a message. */
    void drawStatusBar(const std::string& msg);

    /** Overwrite the top status bar with session info. */
    void drawTopBar();

    /**
     * @brief Read a line of input into a field at (y,x) inside win.
     * @param win     Target window
     * @param y       Row inside win
     * @param x       Column inside win
     * @param maxLen  Maximum characters accepted
     * @param masked  If true, echo '*' instead of the real character
     * @return The entered string
     */
    std::string getInput(WINDOW* win, int y, int x, int maxLen, bool masked = false);

    /**
     * @brief Display a centred red modal with an error message.
     * @param msg Error text (newlines allowed)
     */
    void showError(const std::string& msg);

    /**
     * @brief Display a centred green modal with a success message.
     * @param msg Success text
     */
    void showSuccess(const std::string& msg);

    /**
     * @brief Display a centred yellow confirm dialog.
     * @param question Question to ask
     * @return true if the user pressed 'y' / 'Y' / Enter
     */
    bool confirm(const std::string& question);

    /**
     * @brief Run a generic arrow-key menu.
     * @param win      Window to render into
     * @param items    Menu item labels
     * @param startY   Row in win where first item is drawn
     * @param startX   Column in win where items start
     * @return Index of selected item, or -1 if ESC pressed
     */
    int runMenu(WINDOW* win,
                const std::vector<std::string>& items,
                int startY, int startX);

    /** Press any key to continue — shown in status bar. */
    void pressAnyKey();

    /** Return current date-time as a short string for the top bar. */
    std::string currentDateTime() const;

    /** Return the current user's role name (empty string if not logged in). */
    std::string roleName() const;

    /** Return true if current user's role matches given name (case-insensitive). */
    bool hasRole(const std::string& role) const;

    /**
     * @brief Paginated display of a table.
     * @param headers  Column header strings
     * @param rows     Table data rows
     * @param title    Box title
     */
    void showTable(const std::string& title,
                   const std::vector<std::string>& headers,
                   const std::vector<std::vector<std::string>>& rows);

    /**
     * @brief Display a key-value detail panel.
     * @param title  Box title
     * @param fields Pairs of (label, value)
     */
    void showDetailPanel(const std::string& title,
                         const std::vector<std::pair<std::string,std::string>>& fields);

    // Formatting helpers
    static std::string truncate(const std::string& s, std::size_t maxLen);
    static std::string studentStatusStr(StudentStatus s);
    static std::string academicStandingStr(AcademicStanding a);
    static std::string courseStatusStr(CourseStatus s);
    static std::string semesterStr(Semester s);
    static std::string enrollmentStatusStr(EnrollmentStatus s);
};

} // namespace sms::app::ui
