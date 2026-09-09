/**
 * @file TUI.cpp
 * @brief Full ncurses Text User Interface for the Student Management System.
 *
 * Build requirement: link against ncurses (-lncurses).
 * Compile with C++20.
 */

#include "TUI.hpp"

#include <ncurses.h>
#include <algorithm>
#include <cassert>
#include <chrono>

#include <ctime>
#include <format>
#include <iomanip>
#include <sstream>

#include <string>
#include <vector>

namespace sms::app::ui {

// ═══════════════════════════════════════════════════════════════════════════════
//  Construction / destruction
// ═══════════════════════════════════════════════════════════════════════════════

TUI::TUI(
    std::shared_ptr<IAuthenticationService> authService,
    std::shared_ptr<IStudentService>        studentService,
    std::shared_ptr<ICourseRepository>      courseRepo,
    std::shared_ptr<IEnrollmentService>     enrollService
)
    : m_auth(std::move(authService))
    , m_students(std::move(studentService))
    , m_courses(std::move(courseRepo))
    , m_enrollments(std::move(enrollService))
{
    initNcurses();
}

TUI::~TUI()
{
    cleanupNcurses();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ncurses lifecycle
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::initNcurses()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);               // hide cursor by default
    set_escdelay(50);          // faster ESC response

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_HEADER,   COLOR_WHITE,  COLOR_BLUE);
        init_pair(CP_SELECTED, COLOR_BLACK,  COLOR_CYAN);
        init_pair(CP_ERROR,    COLOR_RED,    COLOR_BLACK);
        init_pair(CP_SUCCESS,  COLOR_GREEN,  COLOR_BLACK);
        init_pair(CP_WARNING,  COLOR_YELLOW, COLOR_BLACK);
    }

    refreshDimensions();
}

void TUI::cleanupNcurses()
{
    endwin();
}

void TUI::refreshDimensions()
{
    getmaxyx(stdscr, m_rows, m_cols);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Main entry point
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::run()
{
    while (true) {
        clear();
        refresh();

        if (!showLogin()) {
            // User quit from the login screen
            break;
        }

        showMainMenu();

        // After logout, clear state and show login again
        m_currentUser.reset();
        m_sessionToken.clear();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Login screen
// ═══════════════════════════════════════════════════════════════════════════════

bool TUI::showLogin()
{
    static const std::vector<std::string> kAsciiArt = {
        " _____ _____ _____   ____   ___ ",
        "/  ___|  _  /  ___| / ___| /   \\",
        "\\ `--.| | | \\ `--.  \\___ \\/ /\\ /",
        " `--. \\ | | |`--. \\  ___) / /_\\/",
        "/\\__/ / \\_/ /\\__/ / /____/ /___/ ",
        "\\____/ \\___/\\____/  \\____\\/      "
    };
    static const std::string kUniversity = "University of Excellence";
    static const std::string kSystem     = "STUDENT MANAGEMENT SYSTEM";

    // Field widths
    static constexpr int kFieldW  = 36;
    static constexpr int kBoxW    = kFieldW + 8;   // label(6) + pad(2) + field
    static constexpr int kBoxH    = 18;

    std::string errorMsg;

    while (true) {
        refreshDimensions();
        clear();

        // ── Draw ASCII art header ──────────────────────────────────────────
        int artTop = 1;
        for (int i = 0; i < static_cast<int>(kAsciiArt.size()); ++i) {
            int x = (m_cols - static_cast<int>(kAsciiArt[i].size())) / 2;
            if (x < 0) x = 0;
            mvprintw(artTop + i, x, "%s", kAsciiArt[i].c_str());
        }

        int headerRow = artTop + static_cast<int>(kAsciiArt.size()) + 1;

        // University name (bold)
        attron(A_BOLD);
        mvprintw(headerRow,
                 (m_cols - static_cast<int>(kUniversity.size())) / 2,
                 "%s", kUniversity.c_str());
        attroff(A_BOLD);

        attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
        mvprintw(headerRow + 1,
                 (m_cols - static_cast<int>(kSystem.size())) / 2,
                 "%s", kSystem.c_str());
        attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

        // ── Login box ─────────────────────────────────────────────────────
        int boxTop = headerRow + 3;
        int boxLeft = (m_cols - kBoxW) / 2;
        if (boxLeft < 0) boxLeft = 0;

        WINDOW* loginWin = newwin(kBoxH, kBoxW, boxTop, boxLeft);
        if (!loginWin) {
            // Terminal too small — bail out
            showError("Terminal too small. Please resize.");
            return false;
        }
        keypad(loginWin, TRUE);
        box(loginWin, 0, 0);
        drawHeader(loginWin, " Login ");

        // Field labels
        mvwprintw(loginWin, 4, 3, "Email   :");
        mvwprintw(loginWin, 7, 3, "Password:");

        // Error message
        if (!errorMsg.empty()) {
            wattron(loginWin, COLOR_PAIR(CP_ERROR) | A_BOLD);
            mvwprintw(loginWin, 12, 3, "%-*s",
                      kBoxW - 6,
                      truncate(errorMsg, static_cast<std::size_t>(kBoxW - 6)).c_str());
            wattroff(loginWin, COLOR_PAIR(CP_ERROR) | A_BOLD);
        }

        mvwprintw(loginWin, kBoxH - 3, 3,
                  "Enter=Login  TAB=Switch field  ESC=Quit");

        wrefresh(loginWin);

        // ── Collect email ──────────────────────────────────────────────────
        curs_set(1);
        std::string email = getInput(loginWin, 5, 13, kFieldW, false);

        if (email.empty()) {
            // ESC was pressed
            curs_set(0);
            werase(loginWin);
            wrefresh(loginWin);
            delwin(loginWin);
            return false;
        }

        // ── Collect password ───────────────────────────────────────────────
        std::string password = getInput(loginWin, 8, 13, kFieldW, true);
        curs_set(0);

        werase(loginWin);
        wrefresh(loginWin);
        delwin(loginWin);

        if (password.empty() && email.empty()) {
            return false;
        }

        // ── Attempt login ──────────────────────────────────────────────────
        try {
            LoginResult result = m_auth->login(email, password, "127.0.0.1", "SMS-TUI/1.0");
            m_currentUser  = result.user;
            m_sessionToken = result.sessionToken;
            return true;
        } catch (const std::exception& ex) {
            errorMsg = ex.what();
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Main menu
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::showMainMenu()
{
    // Build role-appropriate menu
    std::vector<std::string>            labels;
    std::vector<std::function<void()>>  actions;

    std::string role = roleName();
    // Convert to lower for comparison
    std::string roleLow = role;
    std::transform(roleLow.begin(), roleLow.end(), roleLow.begin(), ::tolower);

    auto addItem = [&](const std::string& label, std::function<void()> fn) {
        labels.push_back(label);
        actions.push_back(std::move(fn));
    };

    // Dashboard is universal
    addItem("  Dashboard", [this]{ showDashboard(); });

    if (roleLow == "administrator" || roleLow == "admin") {
        addItem("  Users",                [this]{ showError("User management coming soon."); });
        addItem("  Students",             [this]{ showStudentsMenu(); });
        addItem("  Courses",              [this]{ showCoursesMenu(); });
        addItem("  Enrollments",          [this]{ showEnrollmentsMenu(); });
        addItem("  Reports",              [this]{ showReportsMenu(); });
        addItem("  System",               [this]{ showError("System settings coming soon."); });
    } else if (roleLow == "registrar") {
        addItem("  Students",             [this]{ showStudentsMenu(); });
        addItem("  Courses (View)",        [this]{ listAllCourses(); });
        addItem("  Enrollments",          [this]{ showEnrollmentsMenu(); });
    } else if (roleLow == "lecturer") {
        addItem("  My Courses",           [this]{ listAllCourses(); });
        addItem("  Grade Management",     [this]{ postGradeForm(); });
    } else {
        // Default: Student self-service
        addItem("  My Profile",           [this]{ showMyProfile(); });
        addItem("  My Enrollments",       [this]{ showMyEnrollments(); });
        addItem("  My Transcript",        [this]{ showMyTranscript(); });
        addItem("  My GPA",               [this]{ showMyGPA(); });
    }

    addItem("  Logout", nullptr);   // sentinel — handled below

    static constexpr int kMenuW = 36;
    int menuH = static_cast<int>(labels.size()) + 6;

    while (true) {
        refreshDimensions();
        clear();
        drawTopBar();
        drawStatusBar("F10=Logout  ESC=Back  Arrow Keys=Navigate  Enter=Select");
        refresh();

        int menuTop  = 2;
        int menuLeft = (m_cols - kMenuW) / 2;
        if (menuLeft < 0) menuLeft = 0;

        WINDOW* menuWin = newwin(menuH, kMenuW, menuTop, menuLeft);
        if (!menuWin) break;
        keypad(menuWin, TRUE);
        box(menuWin, 0, 0);
        drawHeader(menuWin, " Main Menu ");
        wrefresh(menuWin);

        int choice = runMenu(menuWin, labels, 2, 1);

        werase(menuWin);
        wrefresh(menuWin);
        delwin(menuWin);

        if (choice < 0) {
            // ESC — treat as logout request if confirmed
            if (confirm("Log out and return to login screen?")) {
                break;
            }
            continue;
        }

        // Last item = Logout
        if (choice == static_cast<int>(labels.size()) - 1) {
            if (confirm("Are you sure you want to log out?")) {
                try { m_auth->logout(m_sessionToken); } catch (...) {}
                break;
            }
            continue;
        }

        // Invoke the chosen action
        try {
            actions[static_cast<std::size_t>(choice)]();
        } catch (const std::exception& ex) {
            showError(std::string("Unexpected error:\n") + ex.what());
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Dashboard
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::showDashboard()
{
    refreshDimensions();
    clear();
    drawTopBar();
    drawStatusBar("Press any key to return");
    refresh();

    // Gather statistics
    int64_t totalStudents  = 0;
    int64_t activeStudents = 0;
    std::size_t totalCourses   = 0;

    try { totalStudents  = m_students->getTotalStudentCount(); } catch (...) {}
    try { activeStudents = m_students->getActiveStudentCount(); } catch (...) {}
    try { totalCourses   = m_courses->readAll().size(); } catch (...) {}

    // ── Outer frame ────────────────────────────────────────────────────────
    static constexpr int kFrameW = 60;
    static constexpr int kFrameH = 18;
    int frameTop  = (m_rows - kFrameH) / 2;
    int frameLeft = (m_cols - kFrameW) / 2;
    if (frameTop  < 2) frameTop  = 2;
    if (frameLeft < 0) frameLeft = 0;

    WINDOW* dash = newwin(kFrameH, kFrameW, frameTop, frameLeft);
    if (!dash) { pressAnyKey(); return; }
    box(dash, 0, 0);
    drawHeader(dash, " Dashboard — System Overview ");

    // ── Stat boxes  (2 x 2 grid) ──────────────────────────────────────────
    struct StatBox {
        std::string label;
        std::string value;
        int colorPair;
    };

    std::vector<StatBox> stats = {
        { "Total Students",   std::to_string(totalStudents),        CP_HEADER   },
        { "Active Students",  std::to_string(activeStudents),       CP_SUCCESS  },
        { "Total Courses",    std::to_string(totalCourses),         CP_WARNING  },
        { "Pending Grades",   "N/A",                                CP_ERROR    },
    };

    // Try to get pending grade count
    try {
        auto pg = m_enrollments->getPendingGrades();
        stats[3].value = std::to_string(pg.size());
    } catch (...) {}

    int boxW = (kFrameW - 6) / 2;
    int boxH = 5;
    int col0 = 2;
    int col1 = col0 + boxW + 2;
    int row0 = 3;
    int row1 = row0 + boxH + 1;

    auto drawStat = [&](int r, int c, const StatBox& sb) {
        WINDOW* bw = derwin(dash, boxH, boxW, r, c);
        if (!bw) return;
        box(bw, 0, 0);
        wattron(bw, A_BOLD);
        mvwprintw(bw, 1, (boxW - static_cast<int>(sb.label.size())) / 2,
                  "%s", sb.label.c_str());
        wattroff(bw, A_BOLD);
        wattron(bw, COLOR_PAIR(sb.colorPair) | A_BOLD);
        mvwprintw(bw, 3, (boxW - static_cast<int>(sb.value.size())) / 2,
                  "%s", sb.value.c_str());
        wattroff(bw, COLOR_PAIR(sb.colorPair) | A_BOLD);
        wrefresh(bw);
        delwin(bw);
    };

    drawStat(row0, col0, stats[0]);
    drawStat(row0, col1, stats[1]);
    drawStat(row1, col0, stats[2]);
    drawStat(row1, col1, stats[3]);

    mvwprintw(dash, kFrameH - 2, 2,
              "Logged in as: %s  |  Role: %s",
              m_currentUser.has_value()
                ? (m_currentUser->firstName() + " " + m_currentUser->lastName()).c_str()
                : "Unknown",
              roleName().c_str());

    wrefresh(dash);
    wgetch(dash);
    werase(dash);
    wrefresh(dash);
    delwin(dash);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Students menu
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::showStudentsMenu()
{
    std::vector<std::string> items = {
        "  List All Students",
        "  Register Student",
        "  Search Students",
        "  View Student by ID",
        "  Update Student Status",
        "  Set Emergency Contact",
        "  Student Statistics",
        "  Back",
    };

    while (true) {
        refreshDimensions();
        clear();
        drawTopBar();
        drawStatusBar("Arrow Keys=Navigate  Enter=Select  ESC=Back");
        refresh();

        int h = static_cast<int>(items.size()) + 4;
        int w = 38;
        WINDOW* win = newwin(h, w, 2, (m_cols - w) / 2);
        if (!win) break;
        keypad(win, TRUE);
        box(win, 0, 0);
        drawHeader(win, " Students ");
        wrefresh(win);

        int choice = runMenu(win, items, 2, 1);
        werase(win); wrefresh(win); delwin(win);

        if (choice < 0 || choice == static_cast<int>(items.size()) - 1) break;

        try {
            switch (choice) {
                case 0: listAllStudents();       break;
                case 1: registerStudentForm();   break;
                case 2: searchStudentsView();    break;
                case 3: viewStudentById();       break;
                case 4: updateStudentStatus();   break;
                case 5: setEmergencyContactForm(); break;
                case 6: showStudentStatistics(); break;
                default: break;
            }
        } catch (const std::exception& ex) {
            showError(ex.what());
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::listAllStudents()
{
    static constexpr int64_t kPageSize = 15;
    int64_t offset = 0;

    while (true) {
        std::vector<Student> page;
        try {
            page = m_students->getActiveStudents(offset, kPageSize);
        } catch (const std::exception& ex) {
            showError(ex.what());
            return;
        }

        std::vector<std::vector<std::string>> rows;
        for (const auto& s : page) {
            rows.push_back({
                std::to_string(s.id()),
                truncate(s.matricNumber(), 14),
                truncate(s.firstName() + " " + s.lastName(), 22),
                truncate(s.email(), 26),
                studentStatusStr(s.status()),
                academicStandingStr(s.academicStanding()),
            });
        }

        showTable(
            std::format(" Active Students (offset={}) ", offset),
            { "ID", "Matric", "Name", "Email", "Status", "Standing" },
            rows
        );

        // Navigation after table
        drawStatusBar("n=Next page  p=Prev page  ESC=Back");
        refresh();
        int ch = getch();
        if (ch == 27 /* ESC */ || ch == 'q' || ch == 'Q') break;
        if ((ch == 'n' || ch == 'N') && static_cast<int64_t>(page.size()) == kPageSize)
            offset += kPageSize;
        if ((ch == 'p' || ch == 'P') && offset >= kPageSize)
            offset -= kPageSize;
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::registerStudentForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 60;
    static constexpr int kH = 20;
    int top  = (m_rows - kH) / 2;
    int left = (m_cols - kW) / 2;
    if (top  < 2) top  = 2;
    if (left < 0) left = 0;

    WINDOW* win = newwin(kH, kW, top, left);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Register Student ");

    // Field labels
    const std::vector<std::pair<int,std::string>> fieldDefs = {
        {3,  "Matric Number "},
        {5,  "First Name    "},
        {7,  "Last Name     "},
        {9,  "Email         "},
        {11, "Phone         "},
        {13, "User ID       "},
    };
    for (auto& [row, label] : fieldDefs) {
        mvwprintw(win, row, 2, "%s:", label.c_str());
    }
    mvwprintw(win, kH - 2, 2, "Tab=Next field  Enter=Submit  ESC=Cancel");
    wrefresh(win);

    static constexpr int kFieldX   = 18;
    static constexpr int kFieldLen = 36;

    curs_set(1);
    std::string matric    = getInput(win, 3,  kFieldX, kFieldLen);
    std::string firstName = getInput(win, 5,  kFieldX, kFieldLen);
    std::string lastName  = getInput(win, 7,  kFieldX, kFieldLen);
    std::string email     = getInput(win, 9,  kFieldX, kFieldLen);
    std::string phone     = getInput(win, 11, kFieldX, kFieldLen);
    std::string userIdStr = getInput(win, 13, kFieldX, kFieldLen);
    curs_set(0);

    werase(win); wrefresh(win); delwin(win);

    if (matric.empty() || firstName.empty() || lastName.empty() ||
        email.empty() || userIdStr.empty())
    {
        showError("Required fields cannot be empty.");
        return;
    }

    int64_t userId = 0;
    try { userId = std::stoll(userIdStr); }
    catch (...) { showError("User ID must be a valid integer."); return; }

    try {
        auto student = m_students->registerStudent(
            userId, matric, firstName, lastName, email, phone
        );
        showSuccess(std::format(
            "Student registered successfully!\nID: {}  Matric: {}",
            student.id(), student.matricNumber()
        ));
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::searchStudentsView()
{
    refreshDimensions();
    clear();
    drawTopBar();
    drawStatusBar("Enter search term and press Enter");
    refresh();

    static constexpr int kW = 54;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Search Students ");
    mvwprintw(win, 3, 2, "Search: ");
    wrefresh(win);

    curs_set(1);
    std::string term = getInput(win, 3, 10, 38);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (term.empty()) return;

    std::vector<Student> results;
    try {
        results = m_students->searchStudents(term);
    } catch (const std::exception& ex) {
        showError(ex.what());
        return;
    }

    if (results.empty()) {
        showError("No students found for: " + term);
        return;
    }

    std::vector<std::vector<std::string>> rows;
    for (const auto& s : results) {
        rows.push_back({
            std::to_string(s.id()),
            truncate(s.matricNumber(), 14),
            truncate(s.firstName() + " " + s.lastName(), 22),
            truncate(s.email(), 26),
            studentStatusStr(s.status()),
        });
    }

    showTable(
        std::format(" Search Results: \"{}\" ({} found) ", term, results.size()),
        { "ID", "Matric", "Name", "Email", "Status" },
        rows
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::viewStudentById()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " View Student ");
    mvwprintw(win, 3, 2, "Student ID: ");
    wrefresh(win);

    curs_set(1);
    std::string idStr = getInput(win, 3, 14, 20);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty()) return;

    int64_t sid = 0;
    try { sid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    std::optional<Student> s;
    try { s = m_students->getStudent(sid); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    if (!s) { showError("Student not found."); return; }

    showDetailPanel(
        std::format(" Student #{} ", s->id()),
        {
            { "ID",               std::to_string(s->id())           },
            { "Matric Number",    s->matricNumber()                  },
            { "First Name",       s->firstName()                     },
            { "Last Name",        s->lastName()                      },
            { "Email",            s->email()                         },
            { "Phone",            s->phoneNumber()                   },
            { "Status",           studentStatusStr(s->status())      },
            { "Acad. Standing",   academicStandingStr(s->academicStanding()) },
            { "Emergency Cntct",  s->emergencyContactName()          },
            { "Emg. Phone",       s->emergencyContactPhone()         },
            { "Guardian",         s->guardianName()                  },
            { "Guardian Phone",   s->guardianPhone()                 },
            { "Guardian Email",   s->guardianEmail()                 },
        }
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::updateStudentStatus()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    // Get student ID
    static constexpr int kInputW = 42;
    static constexpr int kInputH = 7;
    WINDOW* iw = newwin(kInputH, kInputW,
                        (m_rows - kInputH) / 2,
                        (m_cols - kInputW) / 2);
    if (!iw) return;
    keypad(iw, TRUE);
    box(iw, 0, 0);
    drawHeader(iw, " Update Student Status ");
    mvwprintw(iw, 3, 2, "Student ID: ");
    wrefresh(iw);

    curs_set(1);
    std::string idStr = getInput(iw, 3, 14, 20);
    curs_set(0);
    werase(iw); wrefresh(iw); delwin(iw);

    if (idStr.empty()) return;
    int64_t sid = 0;
    try { sid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    // Action sub-menu
    std::vector<std::string> actions = {
        "  Suspend Student",
        "  Reactivate Student",
        "  Graduate Student",
        "  Cancel",
    };

    int aw = 32, ah = static_cast<int>(actions.size()) + 4;
    WINDOW* aw_win = newwin(ah, aw,
                            (m_rows - ah) / 2,
                            (m_cols - aw) / 2);
    if (!aw_win) return;
    keypad(aw_win, TRUE);
    box(aw_win, 0, 0);
    drawHeader(aw_win, " Choose Action ");
    wrefresh(aw_win);

    int choice = runMenu(aw_win, actions, 2, 1);
    werase(aw_win); wrefresh(aw_win); delwin(aw_win);

    if (choice < 0 || choice == 3) return;

    try {
        switch (choice) {
            case 0:
                if (confirm("Suspend student " + idStr + "?")) {
                    m_students->suspendStudent(sid, "Manual suspension via TUI");
                    showSuccess("Student suspended.");
                }
                break;
            case 1:
                if (confirm("Reactivate student " + idStr + "?")) {
                    m_students->reactivateStudent(sid);
                    showSuccess("Student reactivated.");
                }
                break;
            case 2:
                if (confirm("Graduate student " + idStr + "?")) {
                    m_students->graduateStudent(sid);
                    showSuccess("Student graduated.");
                }
                break;
        }
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::setEmergencyContactForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 56;
    static constexpr int kH = 12;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Set Emergency Contact ");

    mvwprintw(win, 3, 2, "Student ID   :");
    mvwprintw(win, 5, 2, "Contact Name :");
    mvwprintw(win, 7, 2, "Contact Phone:");
    mvwprintw(win, kH - 2, 2, "Enter=Submit  ESC=Cancel");
    wrefresh(win);

    static constexpr int kFX = 17;
    static constexpr int kFL = 34;

    curs_set(1);
    std::string idStr = getInput(win, 3, kFX, kFL);
    std::string name  = getInput(win, 5, kFX, kFL);
    std::string phone = getInput(win, 7, kFX, kFL);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty() || name.empty() || phone.empty()) return;

    int64_t sid = 0;
    try { sid = std::stoll(idStr); }
    catch (...) { showError("Invalid Student ID."); return; }

    try {
        m_students->setEmergencyContact(sid, name, phone);
        showSuccess("Emergency contact updated.");
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showStudentStatistics()
{
    std::string statsJson;
    try {
        statsJson = m_students->getStudentStatistics();
    } catch (const std::exception& ex) {
        showError(ex.what());
        return;
    }

    refreshDimensions();
    clear();
    drawTopBar();
    drawStatusBar("Press any key to return");
    refresh();

    static constexpr int kW = 64;
    int kH = std::min(m_rows - 4, 24);
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) { pressAnyKey(); return; }
    box(win, 0, 0);
    drawHeader(win, " Student Statistics ");

    // Pretty-print the JSON string line by line
    std::istringstream iss(statsJson);
    std::string line;
    int row = 2;
    while (std::getline(iss, line) && row < kH - 2) {
        mvwprintw(win, row++, 2, "%s", truncate(line, static_cast<std::size_t>(kW - 4)).c_str());
    }

    wrefresh(win);
    wgetch(win);
    werase(win); wrefresh(win); delwin(win);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Courses menu
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::showCoursesMenu()
{
    std::vector<std::string> items = {
        "  List All Courses",
        "  Add Course",
        "  Search Courses",
        "  View Course Details",
        "  Update Course",
        "  Back",
    };

    while (true) {
        refreshDimensions();
        clear();
        drawTopBar();
        drawStatusBar("Arrow Keys=Navigate  Enter=Select  ESC=Back");
        refresh();

        int h = static_cast<int>(items.size()) + 4;
        int w = 36;
        WINDOW* win = newwin(h, w, 2, (m_cols - w) / 2);
        if (!win) break;
        keypad(win, TRUE);
        box(win, 0, 0);
        drawHeader(win, " Courses ");
        wrefresh(win);

        int choice = runMenu(win, items, 2, 1);
        werase(win); wrefresh(win); delwin(win);

        if (choice < 0 || choice == static_cast<int>(items.size()) - 1) break;

        try {
            switch (choice) {
                case 0: listAllCourses();    break;
                case 1: addCourseForm();     break;
                case 2: searchCoursesView(); break;
                case 3: viewCourseDetails(); break;
                case 4: updateCourseForm();  break;
            }
        } catch (const std::exception& ex) {
            showError(ex.what());
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::listAllCourses()
{
    std::vector<Course> all;
    try { all = m_courses->readAll(); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    static constexpr std::size_t kPage = 15;
    std::size_t offset = 0;

    while (true) {
        std::size_t end = std::min(offset + kPage, all.size());
        std::vector<std::vector<std::string>> rows;
        for (std::size_t i = offset; i < end; ++i) {
            const auto& c = all[i];
            rows.push_back({
                std::to_string(c.id()),
                truncate(c.code(), 8),
                truncate(c.title(), 26),
                std::to_string(c.creditUnits()),
                std::to_string(c.capacity()),
                std::to_string(c.enrolledCount()),
                courseStatusStr(c.status()),
            });
        }

        showTable(
            std::format(" All Courses ({}/{}) ", end, all.size()),
            { "ID", "Code", "Title", "Cr", "Cap", "Enr", "Status" },
            rows
        );

        drawStatusBar("n=Next  p=Prev  ESC=Back");
        refresh();
        int ch = getch();
        if (ch == 27 || ch == 'q' || ch == 'Q') break;
        if ((ch == 'n' || ch == 'N') && end < all.size()) offset += kPage;
        if ((ch == 'p' || ch == 'P') && offset >= kPage)  offset -= kPage;
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::addCourseForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 60;
    static constexpr int kH = 22;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Add Course ");

    mvwprintw(win,  3, 2, "Code        :");
    mvwprintw(win,  5, 2, "Title       :");
    mvwprintw(win,  7, 2, "Description :");
    mvwprintw(win,  9, 2, "Credits     :");
    mvwprintw(win, 11, 2, "Capacity    :");
    mvwprintw(win, 13, 2, "Semester    (FALL/SPRING/SUMMER):");
    mvwprintw(win, 15, 2, "Year        :");
    mvwprintw(win, 17, 2, "Location    :");
    mvwprintw(win, kH - 2, 2, "Enter=Submit  ESC=Cancel");
    wrefresh(win);

    static constexpr int kFX = 16;
    static constexpr int kFL = 40;

    curs_set(1);
    std::string code   = getInput(win,  3, kFX, kFL);
    std::string title  = getInput(win,  5, kFX, kFL);
    std::string desc   = getInput(win,  7, kFX, kFL);
    std::string creds  = getInput(win,  9, kFX, kFL);
    std::string cap    = getInput(win, 11, kFX, kFL);
    std::string sem    = getInput(win, 13, kFX, kFL);
    std::string yr     = getInput(win, 15, kFX, kFL);
    std::string loc    = getInput(win, 17, kFX, kFL);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (code.empty() || title.empty() || creds.empty()) {
        showError("Code, title, and credits are required.");
        return;
    }

    Course c;
    c.setCode(code);
    c.setTitle(title);
    c.setDescription(desc);
    try { c.setCreditUnits(std::stoi(creds)); } catch (...) { showError("Invalid credits."); return; }
    try { c.setCapacity(std::stoi(cap));       } catch (...) { c.setCapacity(30); }
    c.setSemester(sms::app::courses::domain::stringToSemester(sem));
    try { c.setYear(std::stoi(yr)); } catch (...) { c.setYear(2026); }
    c.setLocation(loc);
    c.setStatus(CourseStatus::DRAFT);

    try {
        auto created = m_courses->create(c);
        showSuccess(std::format("Course created!\nID: {}  Code: {}", created.id(), created.code()));
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::searchCoursesView()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 50;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Search Courses ");
    mvwprintw(win, 3, 2, "Search: ");
    wrefresh(win);

    curs_set(1);
    std::string term = getInput(win, 3, 10, 34);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (term.empty()) return;

    std::vector<Course> results;
    try { results = m_courses->search(term); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    if (results.empty()) { showError("No courses found."); return; }

    std::vector<std::vector<std::string>> rows;
    for (const auto& c : results) {
        rows.push_back({
            std::to_string(c.id()),
            truncate(c.code(), 8),
            truncate(c.title(), 28),
            std::to_string(c.creditUnits()),
            courseStatusStr(c.status()),
        });
    }

    showTable(
        std::format(" Course Search: \"{}\" ({} found) ", term, results.size()),
        { "ID", "Code", "Title", "Cr", "Status" },
        rows
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::viewCourseDetails()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " View Course ");
    mvwprintw(win, 3, 2, "Course ID: ");
    wrefresh(win);

    curs_set(1);
    std::string idStr = getInput(win, 3, 13, 22);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty()) return;
    int64_t cid = 0;
    try { cid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    std::optional<Course> c;
    try { c = m_courses->read(cid); }
    catch (const std::exception& ex) { showError(ex.what()); return; }
    if (!c) { showError("Course not found."); return; }

    showDetailPanel(
        std::format(" Course #{} ", c->id()),
        {
            { "ID",           std::to_string(c->id())           },
            { "Code",         c->code()                         },
            { "Title",        c->title()                        },
            { "Description",  truncate(c->description(), 38)   },
            { "Credits",      std::to_string(c->creditUnits())  },
            { "Capacity",     std::to_string(c->capacity())     },
            { "Enrolled",     std::to_string(c->enrolledCount()) },
            { "Status",       courseStatusStr(c->status())      },
            { "Semester",     semesterStr(c->semester())        },
            { "Year",         std::to_string(c->year())        },
            { "Location",     c->location()                     },
            { "Schedule",     c->schedule()                     },
        }
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::updateCourseForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    // First: get course ID
    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* iw = newwin(kH, kW,
                        (m_rows - kH) / 2,
                        (m_cols - kW) / 2);
    if (!iw) return;
    keypad(iw, TRUE);
    box(iw, 0, 0);
    drawHeader(iw, " Update Course ");
    mvwprintw(iw, 3, 2, "Course ID: ");
    wrefresh(iw);

    curs_set(1);
    std::string idStr = getInput(iw, 3, 13, 22);
    curs_set(0);
    werase(iw); wrefresh(iw); delwin(iw);

    if (idStr.empty()) return;
    int64_t cid = 0;
    try { cid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    std::optional<Course> existing;
    try { existing = m_courses->read(cid); }
    catch (const std::exception& ex) { showError(ex.what()); return; }
    if (!existing) { showError("Course not found."); return; }

    // Edit form pre-filled
    static constexpr int kEW = 62;
    static constexpr int kEH = 18;
    WINDOW* ew = newwin(kEH, kEW,
                        (m_rows - kEH) / 2,
                        (m_cols - kEW) / 2);
    if (!ew) return;
    keypad(ew, TRUE);
    box(ew, 0, 0);
    drawHeader(ew, std::format(" Update Course #{} ", cid));

    mvwprintw(ew,  3, 2, "Title       :");
    mvwprintw(ew,  5, 2, "Credits     :");
    mvwprintw(ew,  7, 2, "Capacity    :");
    mvwprintw(ew,  9, 2, "Location    :");
    mvwprintw(ew, 11, 2, "Schedule    :");
    mvwprintw(ew, 13, 2, "Status (DRAFT/ACTIVE/ARCHIVED/CANCELLED):");
    mvwprintw(ew, kEH - 2, 2, "Enter=Submit  ESC=Cancel");
    wrefresh(ew);

    static constexpr int kFX = 16;
    static constexpr int kFL = 42;

    // Pre-fill current values as hint
    mvwprintw(ew, 3,  kFX, "%s", existing->title().c_str());
    mvwprintw(ew, 5,  kFX, "%d", existing->creditUnits());
    mvwprintw(ew, 7,  kFX, "%d", existing->capacity());
    mvwprintw(ew, 9,  kFX, "%s", existing->location().c_str());
    mvwprintw(ew, 11, kFX, "%s", existing->schedule().c_str());
    wrefresh(ew);

    curs_set(1);
    std::string title    = getInput(ew,  3, kFX, kFL);
    std::string creds    = getInput(ew,  5, kFX, kFL);
    std::string cap      = getInput(ew,  7, kFX, kFL);
    std::string loc      = getInput(ew,  9, kFX, kFL);
    std::string sched    = getInput(ew, 11, kFX, kFL);
    std::string statStr  = getInput(ew, 13, kFX, kFL);
    curs_set(0);
    werase(ew); wrefresh(ew); delwin(ew);

    Course updated = *existing;
    if (!title.empty())   updated.setTitle(title);
    if (!creds.empty())   { try { updated.setCreditUnits(std::stoi(creds)); } catch (...) {} }
    if (!cap.empty())     { try { updated.setCapacity(std::stoi(cap));      } catch (...) {} }
    if (!loc.empty())     updated.setLocation(loc);
    if (!sched.empty())   updated.setSchedule(sched);
    if (!statStr.empty()) updated.setStatus(sms::app::courses::domain::stringToStatus(statStr));

    try {
        m_courses->update(updated);
        showSuccess(std::format("Course #{} updated successfully.", cid));
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Enrollments menu
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::showEnrollmentsMenu()
{
    std::vector<std::string> items = {
        "  Enroll Student in Course",
        "  List Student Enrollments",
        "  List Course Enrollments",
        "  Approve Enrollment",
        "  Drop / Withdraw",
        "  Post Grade",
        "  View Student GPA",
        "  View Transcript",
        "  Pending Grades",
        "  Back",
    };

    while (true) {
        refreshDimensions();
        clear();
        drawTopBar();
        drawStatusBar("Arrow Keys=Navigate  Enter=Select  ESC=Back");
        refresh();

        int h = static_cast<int>(items.size()) + 4;
        int w = 36;
        WINDOW* win = newwin(h, w, 2, (m_cols - w) / 2);
        if (!win) break;
        keypad(win, TRUE);
        box(win, 0, 0);
        drawHeader(win, " Enrollments ");
        wrefresh(win);

        int choice = runMenu(win, items, 2, 1);
        werase(win); wrefresh(win); delwin(win);

        if (choice < 0 || choice == static_cast<int>(items.size()) - 1) break;

        try {
            switch (choice) {
                case 0: enrollStudentForm();       break;
                case 1: listStudentEnrollments();  break;
                case 2: listCourseEnrollments();   break;
                case 3: approveEnrollmentForm();   break;
                case 4: dropWithdrawForm();         break;
                case 5: postGradeForm();            break;
                case 6: viewStudentGPA();           break;
                case 7: viewTranscript();           break;
                case 8: showPendingGrades();        break;
            }
        } catch (const std::exception& ex) {
            showError(ex.what());
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::enrollStudentForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 56;
    static constexpr int kH = 14;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Enroll Student in Course ");

    mvwprintw(win, 3, 2, "Student ID :");
    mvwprintw(win, 5, 2, "Course ID  :");
    mvwprintw(win, 7, 2, "Semester   (FALL/SPRING/SUMMER):");
    mvwprintw(win, 9, 2, "Year       :");
    mvwprintw(win, kH - 2, 2, "Enter=Submit  ESC=Cancel");
    wrefresh(win);

    static constexpr int kFX = 15;
    static constexpr int kFL = 36;

    curs_set(1);
    std::string sidStr = getInput(win, 3, kFX, kFL);
    std::string cidStr = getInput(win, 5, kFX, kFL);
    std::string sem    = getInput(win, 7, kFX, kFL);
    std::string yrStr  = getInput(win, 9, kFX, kFL);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (sidStr.empty() || cidStr.empty() || sem.empty()) return;

    int64_t sid = 0, cid = 0;
    int     yr  = 2026;
    try { sid = std::stoll(sidStr); } catch (...) { showError("Invalid Student ID."); return; }
    try { cid = std::stoll(cidStr); } catch (...) { showError("Invalid Course ID.");  return; }
    try { yr  = std::stoi(yrStr);   } catch (...) {}

    try {
        auto enroll = m_enrollments->enrollStudent(sid, cid, sem, yr);
        showSuccess(std::format(
            "Enrollment created!\nEnrollment ID: {}  Status: {}",
            enroll.id(), enrollmentStatusStr(enroll.status())
        ));
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::listStudentEnrollments()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Student Enrollments ");
    mvwprintw(win, 3, 2, "Student ID: ");
    wrefresh(win);

    curs_set(1);
    std::string idStr = getInput(win, 3, 14, 22);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty()) return;
    int64_t sid = 0;
    try { sid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    std::vector<Enrollment> list;
    try { list = m_enrollments->getStudentEnrollments(sid); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    if (list.empty()) { showError("No enrollments found."); return; }

    std::vector<std::vector<std::string>> rows;
    for (const auto& e : list) {
        rows.push_back({
            std::to_string(e.id()),
            std::to_string(e.courseId()),
            e.semester(),
            std::to_string(e.year()),
            enrollmentStatusStr(e.status()),
            e.gradePosted() ? std::format("{:.1f}", e.grade()) : "—",
            e.letterGrade().empty() ? "—" : e.letterGrade(),
        });
    }

    showTable(
        std::format(" Enrollments for Student #{} ", sid),
        { "EnrID", "CrsID", "Semester", "Year", "Status", "Grade", "Letter" },
        rows
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::listCourseEnrollments()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Course Enrollments ");
    mvwprintw(win, 3, 2, "Course ID: ");
    wrefresh(win);

    curs_set(1);
    std::string idStr = getInput(win, 3, 13, 22);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty()) return;
    int64_t cid = 0;
    try { cid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    std::vector<Enrollment> list;
    try { list = m_enrollments->getCourseEnrollments(cid); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    if (list.empty()) { showError("No enrollments for this course."); return; }

    std::vector<std::vector<std::string>> rows;
    for (const auto& e : list) {
        rows.push_back({
            std::to_string(e.id()),
            std::to_string(e.studentId()),
            e.semester(),
            std::to_string(e.year()),
            enrollmentStatusStr(e.status()),
            e.gradePosted() ? std::format("{:.1f}", e.grade()) : "—",
        });
    }

    showTable(
        std::format(" Enrollments for Course #{} ", cid),
        { "EnrID", "StuID", "Semester", "Year", "Status", "Grade" },
        rows
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::approveEnrollmentForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Approve Enrollment ");
    mvwprintw(win, 3, 2, "Enrollment ID: ");
    wrefresh(win);

    curs_set(1);
    std::string idStr = getInput(win, 3, 17, 18);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty()) return;
    int64_t eid = 0;
    try { eid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    if (!confirm(std::format("Approve enrollment #{}?", eid))) return;

    try {
        m_enrollments->approveEnrollment(eid);
        showSuccess(std::format("Enrollment #{} approved.", eid));
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::dropWithdrawForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 44;
    static constexpr int kH = 9;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Drop / Withdraw ");
    mvwprintw(win, 3, 2, "Enrollment ID: ");
    mvwprintw(win, 5, 2, "Action (D=Drop, W=Withdraw): ");
    wrefresh(win);

    curs_set(1);
    std::string idStr  = getInput(win, 3, 17, 18);
    std::string action = getInput(win, 5, 31, 2);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty() || action.empty()) return;
    int64_t eid = 0;
    try { eid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    char act = static_cast<char>(std::toupper(static_cast<unsigned char>(action[0])));
    if (act != 'D' && act != 'W') { showError("Enter D for Drop or W for Withdraw."); return; }

    std::string verb = (act == 'D') ? "Drop" : "Withdraw from";
    if (!confirm(std::format("{} enrollment #{}?", verb, eid))) return;

    try {
        if (act == 'D')
            m_enrollments->dropCourse(eid);
        else
            m_enrollments->withdrawFromCourse(eid);

        showSuccess(std::format("Enrollment #{} {}.", eid,
                                act == 'D' ? "dropped" : "withdrawn"));
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::postGradeForm()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 48;
    static constexpr int kH = 9;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Post Grade ");
    mvwprintw(win, 3, 2, "Enrollment ID       :");
    mvwprintw(win, 5, 2, "Numeric Grade (0-100):");
    wrefresh(win);

    curs_set(1);
    std::string eidStr   = getInput(win, 3, 23, 16);
    std::string gradeStr = getInput(win, 5, 23, 6);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (eidStr.empty() || gradeStr.empty()) return;

    int64_t eid   = 0;
    double  grade = 0.0;
    try { eid   = std::stoll(eidStr);   } catch (...) { showError("Invalid enrollment ID."); return; }
    try { grade = std::stod(gradeStr);  } catch (...) { showError("Invalid grade value.");   return; }

    if (grade < 0.0 || grade > 100.0) { showError("Grade must be between 0 and 100."); return; }

    if (!confirm(std::format("Post grade {:.1f} for enrollment #{}?", grade, eid))) return;

    try {
        m_enrollments->postGrade(eid, grade);
        showSuccess(std::format("Grade {:.1f} posted for enrollment #{}.", grade, eid));
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::viewStudentGPA()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " View Student GPA ");
    mvwprintw(win, 3, 2, "Student ID: ");
    wrefresh(win);

    curs_set(1);
    std::string idStr = getInput(win, 3, 14, 22);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty()) return;
    int64_t sid = 0;
    try { sid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    double gpa = 0.0;
    try { gpa = m_enrollments->getStudentGPA(sid); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    showSuccess(std::format("GPA for Student #{}: {:.2f}", sid, gpa));
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::viewTranscript()
{
    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kW = 42;
    static constexpr int kH = 7;
    WINDOW* win = newwin(kH, kW,
                         (m_rows - kH) / 2,
                         (m_cols - kW) / 2);
    if (!win) return;
    keypad(win, TRUE);
    box(win, 0, 0);
    drawHeader(win, " Generate Transcript ");
    mvwprintw(win, 3, 2, "Student ID: ");
    wrefresh(win);

    curs_set(1);
    std::string idStr = getInput(win, 3, 14, 22);
    curs_set(0);
    werase(win); wrefresh(win); delwin(win);

    if (idStr.empty()) return;
    int64_t sid = 0;
    try { sid = std::stoll(idStr); }
    catch (...) { showError("Invalid ID."); return; }

    std::string transcript;
    try { transcript = m_enrollments->generateTranscript(sid); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    // Display in a scrollable panel
    refreshDimensions();
    clear();
    drawTopBar();
    drawStatusBar("Press any key to return");
    refresh();

    int tw = std::min(m_cols - 4, 72);
    int th = std::min(m_rows - 4, 30);
    WINDOW* tw_win = newwin(th, tw,
                            (m_rows - th) / 2,
                            (m_cols - tw) / 2);
    if (!tw_win) { pressAnyKey(); return; }
    box(tw_win, 0, 0);
    drawHeader(tw_win, std::format(" Transcript — Student #{} ", sid));

    std::istringstream iss(transcript);
    std::string line;
    int row = 2;
    while (std::getline(iss, line) && row < th - 2) {
        mvwprintw(tw_win, row++, 2, "%s",
                  truncate(line, static_cast<std::size_t>(tw - 4)).c_str());
    }

    wrefresh(tw_win);
    wgetch(tw_win);
    werase(tw_win); wrefresh(tw_win); delwin(tw_win);
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showPendingGrades()
{
    std::vector<Enrollment> pending;
    try { pending = m_enrollments->getPendingGrades(); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    if (pending.empty()) { showSuccess("No pending grades."); return; }

    std::vector<std::vector<std::string>> rows;
    for (const auto& e : pending) {
        rows.push_back({
            std::to_string(e.id()),
            std::to_string(e.studentId()),
            std::to_string(e.courseId()),
            e.semester(),
            std::to_string(e.year()),
            enrollmentStatusStr(e.status()),
        });
    }

    showTable(
        std::format(" Pending Grades ({} enrollments) ", pending.size()),
        { "EnrID", "StuID", "CrsID", "Semester", "Year", "Status" },
        rows
    );
    pressAnyKey();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Student self-service screens
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::showMyProfile()
{
    if (!m_currentUser) { showError("Not logged in."); return; }

    std::optional<Student> s;
    try { s = m_students->getStudentByUserId(m_currentUser->id()); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    if (!s) {
        showError("No student record linked to your account.\nContact the registrar.");
        return;
    }

    showDetailPanel(
        " My Profile ",
        {
            { "ID",             std::to_string(s->id())                    },
            { "Matric Number",  s->matricNumber()                           },
            { "Full Name",      s->firstName() + " " + s->lastName()       },
            { "Email",          s->email()                                  },
            { "Phone",          s->phoneNumber()                            },
            { "Status",         studentStatusStr(s->status())               },
            { "Acad. Standing", academicStandingStr(s->academicStanding())  },
            { "Emg. Contact",   s->emergencyContactName()                   },
            { "Emg. Phone",     s->emergencyContactPhone()                  },
        }
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showMyEnrollments()
{
    if (!m_currentUser) { showError("Not logged in."); return; }

    std::optional<Student> s;
    try { s = m_students->getStudentByUserId(m_currentUser->id()); }
    catch (const std::exception& ex) { showError(ex.what()); return; }
    if (!s) { showError("No student record found."); return; }

    std::vector<Enrollment> list;
    try { list = m_enrollments->getStudentEnrollments(s->id()); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    if (list.empty()) { showError("You have no enrollments."); return; }

    std::vector<std::vector<std::string>> rows;
    for (const auto& e : list) {
        rows.push_back({
            std::to_string(e.id()),
            std::to_string(e.courseId()),
            e.semester(),
            std::to_string(e.year()),
            enrollmentStatusStr(e.status()),
            e.gradePosted() ? std::format("{:.1f}", e.grade()) : "—",
            e.letterGrade().empty() ? "—" : e.letterGrade(),
        });
    }

    showTable(
        " My Enrollments ",
        { "EnrID", "CrsID", "Semester", "Year", "Status", "Grade", "Letter" },
        rows
    );
    pressAnyKey();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showMyTranscript()
{
    if (!m_currentUser) { showError("Not logged in."); return; }

    std::optional<Student> s;
    try { s = m_students->getStudentByUserId(m_currentUser->id()); }
    catch (const std::exception& ex) { showError(ex.what()); return; }
    if (!s) { showError("No student record found."); return; }

    // Reuse viewTranscript logic but pass own student ID
    std::string transcript;
    try { transcript = m_enrollments->generateTranscript(s->id()); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    refreshDimensions();
    clear();
    drawTopBar();
    drawStatusBar("Press any key to return");
    refresh();

    int tw = std::min(m_cols - 4, 72);
    int th = std::min(m_rows - 4, 30);
    WINDOW* tw_win = newwin(th, tw,
                            (m_rows - th) / 2,
                            (m_cols - tw) / 2);
    if (!tw_win) { pressAnyKey(); return; }
    box(tw_win, 0, 0);
    drawHeader(tw_win, " My Transcript ");

    std::istringstream iss(transcript);
    std::string line;
    int row = 2;
    while (std::getline(iss, line) && row < th - 2) {
        mvwprintw(tw_win, row++, 2, "%s",
                  truncate(line, static_cast<std::size_t>(tw - 4)).c_str());
    }

    wrefresh(tw_win);
    wgetch(tw_win);
    werase(tw_win); wrefresh(tw_win); delwin(tw_win);
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showMyGPA()
{
    if (!m_currentUser) { showError("Not logged in."); return; }

    std::optional<Student> s;
    try { s = m_students->getStudentByUserId(m_currentUser->id()); }
    catch (const std::exception& ex) { showError(ex.what()); return; }
    if (!s) { showError("No student record found."); return; }

    double gpa = 0.0;
    try { gpa = m_enrollments->getStudentGPA(s->id()); }
    catch (const std::exception& ex) { showError(ex.what()); return; }

    showSuccess(std::format("Your current GPA: {:.2f}", gpa));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Reports menu (Admin)
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::showReportsMenu()
{
    std::vector<std::string> items = {
        "  Student Statistics",
        "  Students on Probation",
        "  Pending Grades Report",
        "  Back",
    };

    while (true) {
        refreshDimensions();
        clear();
        drawTopBar();
        drawStatusBar("Arrow Keys=Navigate  Enter=Select  ESC=Back");
        refresh();

        int h = static_cast<int>(items.size()) + 4;
        int w = 36;
        WINDOW* win = newwin(h, w, 2, (m_cols - w) / 2);
        if (!win) break;
        keypad(win, TRUE);
        box(win, 0, 0);
        drawHeader(win, " Reports ");
        wrefresh(win);

        int choice = runMenu(win, items, 2, 1);
        werase(win); wrefresh(win); delwin(win);

        if (choice < 0 || choice == static_cast<int>(items.size()) - 1) break;

        try {
            switch (choice) {
                case 0: showStudentStatistics(); break;
                case 1: {
                    // Students on probation
                    std::vector<Student> prob;
                    try { prob = m_students->getStudentsOnProbation(); }
                    catch (const std::exception& ex) { showError(ex.what()); break; }

                    if (prob.empty()) { showSuccess("No students on probation."); break; }

                    std::vector<std::vector<std::string>> rows;
                    for (const auto& s : prob) {
                        rows.push_back({
                            std::to_string(s.id()),
                            truncate(s.matricNumber(), 14),
                            truncate(s.firstName() + " " + s.lastName(), 24),
                            truncate(s.email(), 26),
                            academicStandingStr(s.academicStanding()),
                        });
                    }
                    showTable(
                        std::format(" Students on Probation ({}) ", prob.size()),
                        { "ID", "Matric", "Name", "Email", "Standing" },
                        rows
                    );
                    pressAnyKey();
                    break;
                }
                case 2: showPendingGrades(); break;
            }
        } catch (const std::exception& ex) {
            showError(ex.what());
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Generic UI helpers
// ═══════════════════════════════════════════════════════════════════════════════

void TUI::drawHeader(WINDOW* win, const std::string& title)
{
    int w = getmaxx(win);
    wattron(win, COLOR_PAIR(CP_HEADER) | A_BOLD);
    // Center the title on row 1
    int x = (w - static_cast<int>(title.size())) / 2;
    if (x < 1) x = 1;
    mvwprintw(win, 1, x, "%s", title.c_str());
    wattroff(win, COLOR_PAIR(CP_HEADER) | A_BOLD);
    // Separator line
    mvwhline(win, 2, 1, ACS_HLINE, w - 2);
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::drawTopBar()
{
    refreshDimensions();
    std::string userName   = m_currentUser
        ? (m_currentUser->firstName() + " " + m_currentUser->lastName())
        : "N/A";
    std::string roleStr    = roleName();
    std::string timeStr    = currentDateTime();

    std::string bar = std::format(
        " SMS v1.0  |  User: {}  |  Role: {}  |  {}",
        userName, roleStr, timeStr
    );

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvhline(0, 0, ' ', m_cols);
    mvprintw(0, 0, "%s", truncate(bar, static_cast<std::size_t>(m_cols - 1)).c_str());
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::drawStatusBar(const std::string& msg)
{
    refreshDimensions();
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvhline(m_rows - 1, 0, ' ', m_cols);
    mvprintw(m_rows - 1, 0, " %s",
             truncate(msg, static_cast<std::size_t>(m_cols - 2)).c_str());
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
    refresh();
}

// ───────────────────────────────────────────────────────────────────────────────

std::string TUI::getInput(WINDOW* win, int y, int x, int maxLen, bool masked)
{
    std::string result;
    result.reserve(static_cast<std::size_t>(maxLen));

    wmove(win, y, x);
    echo();
    curs_set(1);

    // Draw empty field box
    wattron(win, A_UNDERLINE);
    for (int i = 0; i < maxLen; ++i) mvwaddch(win, y, x + i, ' ');
    wmove(win, y, x);
    wattroff(win, A_UNDERLINE);
    wrefresh(win);

    noecho();   // We do our own echo for masking support

    while (true) {
        int ch = wgetch(win);

        if (ch == 27 /* ESC */) {
            result.clear();
            break;
        }
        if (ch == '\n' || ch == KEY_ENTER || ch == '\r' || ch == '\t') {
            break;
        }
        if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) && !result.empty()) {
            result.pop_back();
            int cur = x + static_cast<int>(result.size());
            wattron(win, A_UNDERLINE);
            mvwaddch(win, y, cur, ' ');
            wattroff(win, A_UNDERLINE);
            wmove(win, y, cur);
            wrefresh(win);
            continue;
        }
        if (ch >= 32 && ch < 127 && static_cast<int>(result.size()) < maxLen) {
            result += static_cast<char>(ch);
            int cur = x + static_cast<int>(result.size()) - 1;
            wattron(win, A_UNDERLINE);
            mvwaddch(win, y, cur, masked ? '*' : static_cast<chtype>(ch));
            wattroff(win, A_UNDERLINE);
            wmove(win, y, cur + 1);
            wrefresh(win);
        }
    }

    curs_set(0);
    return result;
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showError(const std::string& msg)
{
    refreshDimensions();

    // Calculate required size
    std::vector<std::string> lines;
    std::istringstream iss(msg);
    std::string line;
    while (std::getline(iss, line)) lines.push_back(line);

    int maxLine = 0;
    for (const auto& l : lines)
        maxLine = std::max(maxLine, static_cast<int>(l.size()));

    int mw = std::max(maxLine + 6, 36);
    int mh = static_cast<int>(lines.size()) + 6;
    mw = std::min(mw, m_cols - 4);
    mh = std::min(mh, m_rows - 4);

    int top  = (m_rows - mh) / 2;
    int left = (m_cols - mw) / 2;
    if (top  < 0) top  = 0;
    if (left < 0) left = 0;

    WINDOW* modal = newwin(mh, mw, top, left);
    if (!modal) return;
    keypad(modal, TRUE);

    wattron(modal, COLOR_PAIR(CP_ERROR));
    box(modal, 0, 0);
    wattron(modal, A_BOLD);
    std::string title = " ERROR ";
    mvwprintw(modal, 1,
              (mw - static_cast<int>(title.size())) / 2,
              "%s", title.c_str());
    wattroff(modal, A_BOLD);
    mvwhline(modal, 2, 1, ACS_HLINE, mw - 2);

    for (int i = 0; i < static_cast<int>(lines.size()) && i + 3 < mh - 2; ++i) {
        mvwprintw(modal, 3 + i, 3, "%s",
                  truncate(lines[static_cast<std::size_t>(i)],
                           static_cast<std::size_t>(mw - 6)).c_str());
    }

    mvwprintw(modal, mh - 2,
              (mw - 20) / 2,
              "  Press any key...  ");
    wattroff(modal, COLOR_PAIR(CP_ERROR));

    wrefresh(modal);
    wgetch(modal);
    werase(modal);
    wrefresh(modal);
    delwin(modal);
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showSuccess(const std::string& msg)
{
    refreshDimensions();

    std::vector<std::string> lines;
    std::istringstream iss(msg);
    std::string line;
    while (std::getline(iss, line)) lines.push_back(line);

    int maxLine = 0;
    for (const auto& l : lines)
        maxLine = std::max(maxLine, static_cast<int>(l.size()));

    int mw = std::max(maxLine + 6, 36);
    int mh = static_cast<int>(lines.size()) + 6;
    mw = std::min(mw, m_cols - 4);
    mh = std::min(mh, m_rows - 4);

    int top  = (m_rows - mh) / 2;
    int left = (m_cols - mw) / 2;
    if (top  < 0) top  = 0;
    if (left < 0) left = 0;

    WINDOW* modal = newwin(mh, mw, top, left);
    if (!modal) return;
    keypad(modal, TRUE);

    wattron(modal, COLOR_PAIR(CP_SUCCESS));
    box(modal, 0, 0);
    wattron(modal, A_BOLD);
    std::string title = " SUCCESS ";
    mvwprintw(modal, 1,
              (mw - static_cast<int>(title.size())) / 2,
              "%s", title.c_str());
    wattroff(modal, A_BOLD);
    mvwhline(modal, 2, 1, ACS_HLINE, mw - 2);

    for (int i = 0; i < static_cast<int>(lines.size()) && i + 3 < mh - 2; ++i) {
        mvwprintw(modal, 3 + i, 3, "%s",
                  truncate(lines[static_cast<std::size_t>(i)],
                           static_cast<std::size_t>(mw - 6)).c_str());
    }

    mvwprintw(modal, mh - 2,
              (mw - 20) / 2,
              "  Press any key...  ");
    wattroff(modal, COLOR_PAIR(CP_SUCCESS));

    wrefresh(modal);
    wgetch(modal);
    werase(modal);
    wrefresh(modal);
    delwin(modal);
}

// ───────────────────────────────────────────────────────────────────────────────

bool TUI::confirm(const std::string& question)
{
    refreshDimensions();

    int mw = std::max(static_cast<int>(question.size()) + 6, 40);
    mw     = std::min(mw, m_cols - 4);
    int mh = 8;

    int top  = (m_rows - mh) / 2;
    int left = (m_cols - mw) / 2;
    if (top  < 0) top  = 0;
    if (left < 0) left = 0;

    WINDOW* modal = newwin(mh, mw, top, left);
    if (!modal) return false;
    keypad(modal, TRUE);

    wattron(modal, COLOR_PAIR(CP_WARNING));
    box(modal, 0, 0);
    wattron(modal, A_BOLD);
    std::string title = " Confirm ";
    mvwprintw(modal, 1,
              (mw - static_cast<int>(title.size())) / 2,
              "%s", title.c_str());
    wattroff(modal, A_BOLD);
    mvwhline(modal, 2, 1, ACS_HLINE, mw - 2);

    mvwprintw(modal, 3, 3, "%s",
              truncate(question, static_cast<std::size_t>(mw - 6)).c_str());

    mvwprintw(modal, mh - 2, 3, "[Y]es   [N]o");
    wattroff(modal, COLOR_PAIR(CP_WARNING));
    wrefresh(modal);

    bool result = false;
    while (true) {
        int ch = wgetch(modal);
        if (ch == 'y' || ch == 'Y' || ch == '\n' || ch == KEY_ENTER) {
            result = true;
            break;
        }
        if (ch == 'n' || ch == 'N' || ch == 27 /* ESC */ || ch == 'q') {
            result = false;
            break;
        }
    }

    werase(modal);
    wrefresh(modal);
    delwin(modal);
    return result;
}

// ───────────────────────────────────────────────────────────────────────────────

int TUI::runMenu(WINDOW* win, const std::vector<std::string>& items,
                 int startY, int startX)
{
    int current = 0;
    int n       = static_cast<int>(items.size());
    int w       = getmaxx(win);
    int itemW   = w - startX - 2;
    if (itemW < 4) itemW = 4;

    auto redraw = [&]() {
        for (int i = 0; i < n; ++i) {
            if (i == current) {
                wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            }
            mvwprintw(win, startY + i, startX, "%-*s",
                      itemW,
                      truncate(items[static_cast<std::size_t>(i)],
                               static_cast<std::size_t>(itemW)).c_str());
            if (i == current) {
                wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            }
        }
        wrefresh(win);
    };

    redraw();

    while (true) {
        int ch = wgetch(win);

        if (ch == KEY_RESIZE) {
            refreshDimensions();
            drawTopBar();
            drawStatusBar("");
            // Caller must re-render the whole screen;
            // returning -2 signals resize to the caller if needed.
            // For simplicity we just redraw the menu item highlight.
            redraw();
            continue;
        }

        switch (ch) {
            case KEY_UP:
            case 'k':
                current = (current - 1 + n) % n;
                redraw();
                break;
            case KEY_DOWN:
            case 'j':
                current = (current + 1) % n;
                redraw();
                break;
            case KEY_HOME:
                current = 0;
                redraw();
                break;
            case KEY_END:
                current = n - 1;
                redraw();
                break;
            case '\n':
            case '\r':
            case KEY_ENTER:
                return current;
            case 27 /* ESC */:
                return -1;
            case KEY_F(10):
                return -1; // Treat F10 as ESC/back at menu level
            default:
                break;
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::pressAnyKey()
{
    drawStatusBar("Press any key to continue...");
    refresh();
    getch();
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showTable(const std::string& title,
                    const std::vector<std::string>& headers,
                    const std::vector<std::vector<std::string>>& rows)
{
    if (headers.empty()) return;

    refreshDimensions();
    clear();
    drawTopBar();
    refresh();

    static constexpr int kPageSize = 15;

    // Compute column widths: max of header width and any cell width, capped.
    std::size_t ncols = headers.size();
    std::vector<int> colW(ncols, 0);

    for (std::size_t c = 0; c < ncols; ++c) {
        colW[c] = static_cast<int>(headers[c].size());
        for (const auto& row : rows) {
            if (c < row.size()) {
                colW[c] = std::max(colW[c], static_cast<int>(row[c].size()));
            }
        }
        colW[c] = std::min(colW[c], 28); // max col width
        colW[c] = std::max(colW[c], 4);  // min col width
    }

    // Total table width = sum of cols + separators + borders
    int totalW = 2; // borders
    for (int w : colW) totalW += w + 3; // " col "
    totalW = std::min(totalW, m_cols);

    int pageCount = rows.empty() ? 1
        : (static_cast<int>(rows.size()) + kPageSize - 1) / kPageSize;
    int page = 0;

    while (true) {
        refreshDimensions();
        clear();
        drawTopBar();

        int visRows = std::min(m_rows - 8, kPageSize);
        int winH    = visRows + 6; // header rows + data + borders
        int winW    = std::min(totalW, m_cols - 2);
        int winTop  = 2;
        int winLeft = (m_cols - winW) / 2;
        if (winLeft < 0) winLeft = 0;

        WINDOW* tw = newwin(winH, winW, winTop, winLeft);
        if (!tw) {
            pressAnyKey();
            return;
        }
        box(tw, 0, 0);
        drawHeader(tw, title);

        // Column header row
        int cx = 1;
        wattron(tw, A_BOLD | A_UNDERLINE);
        for (std::size_t c = 0; c < ncols; ++c) {
            mvwprintw(tw, 3, cx, " %-*s",
                      colW[c],
                      truncate(headers[c], static_cast<std::size_t>(colW[c])).c_str());
            cx += colW[c] + 3;
            if (cx >= winW - 1) break;
        }
        wattroff(tw, A_BOLD | A_UNDERLINE);

        // Separator
        mvwhline(tw, 4, 1, ACS_HLINE, winW - 2);

        // Data rows
        std::size_t rowStart = static_cast<std::size_t>(page * kPageSize);
        std::size_t rowEnd   = std::min(rowStart + static_cast<std::size_t>(visRows), rows.size());

        for (std::size_t r = rowStart; r < rowEnd; ++r) {
            int wr = 5 + static_cast<int>(r - rowStart);
            cx = 1;
            const auto& row = rows[r];
            for (std::size_t c = 0; c < ncols; ++c) {
                std::string cell = (c < row.size()) ? row[c] : "";
                mvwprintw(tw, wr, cx, " %-*s",
                          colW[c],
                          truncate(cell, static_cast<std::size_t>(colW[c])).c_str());
                cx += colW[c] + 3;
                if (cx >= winW - 1) break;
            }
        }

        // Footer
        std::string pageInfo = std::format(
            " Page {}/{} | {} rows total ",
            page + 1, pageCount, rows.size()
        );
        wattron(tw, A_DIM);
        mvwprintw(tw, winH - 2, 2, "%s",
                  truncate(pageInfo, static_cast<std::size_t>(winW - 4)).c_str());
        wattroff(tw, A_DIM);

        drawStatusBar("n=Next page  p=Prev page  ESC=Return");
        wrefresh(tw);

        int ch = wgetch(tw);
        werase(tw); wrefresh(tw); delwin(tw);

        if (ch == 27 || ch == 'q' || ch == 'Q') break;
        if ((ch == 'n' || ch == 'N' || ch == KEY_NPAGE) && page + 1 < pageCount)
            ++page;
        if ((ch == 'p' || ch == 'P' || ch == KEY_PPAGE) && page > 0)
            --page;
    }
}

// ───────────────────────────────────────────────────────────────────────────────

void TUI::showDetailPanel(const std::string& title,
                           const std::vector<std::pair<std::string,std::string>>& fields)
{
    refreshDimensions();
    clear();
    drawTopBar();

    int maxLabel = 0;
    for (const auto& [label, _] : fields)
        maxLabel = std::max(maxLabel, static_cast<int>(label.size()));

    int valueW = std::min(m_cols - maxLabel - 10, 46);
    int panelW = maxLabel + valueW + 8;
    panelW = std::min(panelW, m_cols - 2);

    int panelH = static_cast<int>(fields.size()) + 6;
    panelH = std::min(panelH, m_rows - 4);

    int top  = (m_rows - panelH) / 2;
    int left = (m_cols - panelW) / 2;
    if (top  < 2) top  = 2;
    if (left < 0) left = 0;

    WINDOW* panel = newwin(panelH, panelW, top, left);
    if (!panel) { pressAnyKey(); return; }
    box(panel, 0, 0);
    drawHeader(panel, title);

    for (int i = 0; i < static_cast<int>(fields.size()) && i + 3 < panelH - 2; ++i) {
        const auto& [label, value] = fields[static_cast<std::size_t>(i)];
        wattron(panel, A_BOLD);
        mvwprintw(panel, 3 + i, 2, "%-*s :", maxLabel, label.c_str());
        wattroff(panel, A_BOLD);
        mvwprintw(panel, 3 + i, maxLabel + 5, "%s",
                  truncate(value, static_cast<std::size_t>(valueW)).c_str());
    }

    mvwhline(panel, panelH - 3, 1, ACS_HLINE, panelW - 2);
    mvwprintw(panel, panelH - 2, 2, "Press any key to return");
    wrefresh(panel);
    wgetch(panel);
    werase(panel); wrefresh(panel); delwin(panel);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Utility / formatting helpers
// ═══════════════════════════════════════════════════════════════════════════════

std::string TUI::truncate(const std::string& s, std::size_t maxLen)
{
    if (s.size() <= maxLen) return s;
    if (maxLen <= 3) return s.substr(0, maxLen);
    return s.substr(0, maxLen - 3) + "...";
}

// ───────────────────────────────────────────────────────────────────────────────

std::string TUI::currentDateTime() const
{
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

// ───────────────────────────────────────────────────────────────────────────────

std::string TUI::roleName() const
{
    if (!m_currentUser) return "";
    return m_currentUser->role().name();
}

// ───────────────────────────────────────────────────────────────────────────────

bool TUI::hasRole(const std::string& role) const
{
    std::string myRole = roleName();
    std::string target = role;
    std::transform(myRole.begin(),   myRole.end(),   myRole.begin(),   ::tolower);
    std::transform(target.begin(),   target.end(),   target.begin(),   ::tolower);
    return myRole == target;
}

// ───────────────────────────────────────────────────────────────────────────────

std::string TUI::studentStatusStr(StudentStatus s)
{
    return sms::app::students::domain::statusToString(s);
}

std::string TUI::academicStandingStr(AcademicStanding a)
{
    return sms::app::students::domain::standingToString(a);
}

std::string TUI::courseStatusStr(CourseStatus s)
{
    return sms::app::courses::domain::statusToString(s);
}

std::string TUI::semesterStr(Semester s)
{
    return sms::app::courses::domain::semesterToString(s);
}

std::string TUI::enrollmentStatusStr(EnrollmentStatus s)
{
    return sms::app::enrollments::domain::statusToString(s);
}

} // namespace sms::app::ui
