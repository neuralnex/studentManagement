# Enterprise Student Database Management System
## Windows User Guide & Setup Instructions

---

## 📌 1. Overview

The **Enterprise Student Database Management System** is a professional desktop application engineered in modern **C++20** featuring a **Qt5 graphical user interface** and a **PostgreSQL relational database** back-end.

### Key Capabilities
* **Role-Based Authentication & Authorization (RBAC)**: Supports Administrator, Registrar, Lecturer, and Student roles with bcrypt-hashed credentials and account locking.
* **Executive Dashboard**: Real-time KPI statistics for Total Students, Active Students, Courses Offered, and Pending Grades.
* **Student Information System**: Searchable directory, enrollment details, emergency contacts, guardian records, and status controls (`ACTIVE`, `SUSPENDED`, `GRADUATED`, `INACTIVE`).
* **Course Catalog**: Course management with code, title, credits, department mapping, capacity, and enrollment tracking.
* **Enrollment & Academic Records (5 Dedicated Tabs)**:
  1. *Enroll Student*: Register students into courses for specific terms/academic years.
  2. *Student Enrollments*: View student records with actions to Approve, Drop, Withdraw, Complete, or Post Grades.
  3. *Course Enrollments*: Course-centric enrollment view and grade submission.
  4. *Pending Grades Queue*: Dedicated workflow for faculty to review and submit numeric grades (0–100) with automatic letter conversion.
  5. *Transcript & GPA Calculator*: Official academic transcript generation and cumulative GPA computation.

---

## 🚀 2. Quick Start on Windows (Method 1: 1-Click Launcher)

This distribution package includes the compiled binary `student_management_system.exe` and a 1-click Windows launcher.

### Requirements
* **Windows 10 (Build 19044+) or Windows 11**
* **WSL2 with WSLg** (standard on modern Windows, renders Linux GUI applications directly as native Windows windows)

### How to Run
1. Extract this zip archive to any folder (e.g. `C:\SMS` or your Desktop).
2. Double-click **`run_sms_windows.bat`**.
3. The Qt5 graphical application window will open directly on your Windows desktop.
4. Log in using any of the default accounts listed in Section 5.

> **Note**: If you do not have WSL installed yet, open Windows PowerShell as Administrator and run:
> ```powershell
> wsl --install
> ```
> Restart your PC when prompted, then double-click `run_sms_windows.bat`.

---

## 💻 3. Running via WSL Terminal (Method 2)

If you prefer using the WSL / Ubuntu terminal on Windows:

1. Open your terminal in this extracted folder:
   ```bash
   cd /mnt/c/path/to/extracted/StudentManagementSystem_Windows
   ```
2. Make sure dependencies are installed in your WSL Ubuntu:
   ```bash
   sudo apt update
   sudo apt install -y qtbase5-dev libpqxx-dev libspdlog-dev libssl-dev
   ```
3. Run the executable:
   ```bash
   ./student_management_system.exe
   ```

---

## 🛠️ 4. Native Windows Compilation with MSVC (Method 3)

If you wish to compile a 100% native Win32/x64 PE binary on Windows using Microsoft Visual Studio:

### Prerequisites
1. **Visual Studio 2022** (Community, Professional, or Enterprise) with **Desktop development with C++** selected.
2. **CMake 3.20+** (included with Visual Studio or downloadable from [cmake.org](https://cmake.org/download/)).
3. **vcpkg** package manager:
   ```cmd
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   ```
4. Install the required C++ dependencies via vcpkg:
   ```cmd
   vcpkg install libpqxx:x64-windows spdlog:x64-windows nlohmann-json:x64-windows openssl:x64-windows qt5-base:x64-windows
   ```

### Building Natively
Run the included batch script:
```cmd
build_windows_msvc.bat
```
Or manually run:
```cmd
cmake -B build_win -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build_win --config Release
```
The native executable will be located in:
`build_win\bin\Release\student_management_system.exe`

---

## 🗄️ 5. Database Setup & Configuration

The application reads its database connection settings from the [`.env`](.env) file in the application directory.

### Pre-Configured Cloud Database
By default, the included `.env` file is already connected to a live Neon Cloud PostgreSQL database with complete schemas and seed data:
```env
DATABASE_URL=postgresql://neondb_owner:npg_mY6Mez0qiFtO@ep-flat-bird-ax02e10o-pooler.c-4.us-east-2.aws.neon.tech/neondb?sslmode=require
```
**You do not need to install local PostgreSQL to test the application immediately.**

### (Optional) Using a Local PostgreSQL Server on Windows
If you prefer running a local database on your Windows machine:
1. Download and install [PostgreSQL for Windows](https://www.postgresql.org/download/windows/).
2. Open **pgAdmin** or `psql` and create the database and user:
   ```sql
   CREATE DATABASE student_db;
   CREATE USER sms_user WITH ENCRYPTED PASSWORD 'sms_password';
   GRANT ALL PRIVILEGES ON DATABASE student_db TO sms_user;
   ```
3. Run the database migration and seed scripts from the `sql/` folder:
   ```cmd
   psql -U postgres -d student_db -f sql/migrations/001_initial_schema.sql
   psql -U postgres -d student_db -f sql/migrations/002_schema_fixes.sql
   psql -U postgres -d student_db -f sql/seed/001_seed_data.sql
   ```
4. Update the `.env` file:
   ```env
   DB_HOST=localhost
   DB_PORT=5432
   DB_NAME=student_db
   DB_USER=sms_user
   DB_PASSWORD=sms_password
   DB_POOL_SIZE=10
   ```

---

## 🔑 6. Default Login Credentials

Use any of these pre-seeded accounts to log in:

| Role | Email | Password | Allowed Operations |
| :--- | :--- | :--- | :--- |
| **Administrator** | `admin@university.edu` | `Admin@123` | Full access to Dashboard, Students, Courses, Enrollments, Grades, Transcripts |
| **Registrar** | `registrar@university.edu` | `Registrar@123` | Student admissions, catalog management, course registration approvals |
| **Lecturer** | `lecturer@university.edu` | `Lecturer@123` | Review course enrollments, grade submissions, academic performance |
| **Student** | `student@university.edu` | `Student@123` | View personal enrollment status, course schedule, cumulative GPA transcript |

---

## 📋 7. Feature Walkthrough & Usage Guide

### 1. Login Screen
* Launch the application to open the **Student Management System — Login** dialog.
* Enter your email and password.
* On successful authentication, a secure session token is generated and the main application window is displayed.

### 2. Dashboard
* Provides real-time metrics summarizing the institution's operations:
  * **Total Students**: Total registered student count.
  * **Active Students**: Students currently in active standing.
  * **Total Courses**: Count of active courses in the curriculum.
  * **Pending Grades**: Number of course enrollments awaiting lecturer grade submission.

### 3. Students Directory
* Access the student list with name, matriculation number, email, and current status.
* **Search Bar**: Real-time filtering by name or matric number.
* **Add Student**: Opens a modal form to input student details, contact numbers, emergency contacts, and guardian details.
* **Status Updates**: Modify status between `ACTIVE`, `SUSPENDED`, `GRADUATED`, and `INACTIVE`.

### 4. Course Catalog
* Comprehensive catalog showing course code (e.g. `CS101`, `CS201`, `CS301`), title, description, credit units, capacity, and current enrollment count.
* Add or update course offerings and assign teaching staff.

### 5. Enrollments Module (5 Tabs)
* **Tab 1 — Enroll Student**: Input the Student ID, Course ID, Semester (e.g. `FALL2026`), and Year to register a student.
* **Tab 2 — Student Enrollments**: Enter a Student ID to view all their enrolled courses. Action buttons allow you to:
  * *Approve*: Transition status from `PENDING` to `APPROVED`/`ACTIVE`.
  * *Drop*: Cancel an enrollment without academic penalty.
  * *Withdraw*: Record formal course withdrawal.
  * *Complete*: Finalize course completion.
  * *Post Grade*: Submit grade directly for this enrollment.
* **Tab 3 — Course Enrollments**: Enter a Course ID to see all students registered for that course.
* **Tab 4 — Pending Grades**: One-click refresh displays all completed courses requiring grades. Click **Post Grade** to enter a score from 0–100.
* **Tab 5 — Transcript & GPA**: Enter a Student ID and click **Load Transcript**. Generates the official course-by-course grade report and displays the student's cumulative GPA.

---

## ❓ 8. Troubleshooting & FAQ

| Problem | Cause | Solution |
| :--- | :--- | :--- |
| **"Database connection pool is unhealthy"** | Invalid network connection or bad `.env` credentials | Check that your computer has internet access to connect to Neon Cloud, or verify your local PostgreSQL credentials in `.env`. |
| **"Cannot connect to X server" (WSL)** | WSLg display forwarding not active | On Windows 10/11, run `wsl --update` in PowerShell as Administrator to ensure WSLg is installed. |
| **Missing Qt libraries in WSL** | Dependencies not installed in WSL distro | Run `wsl sudo apt update && wsl sudo apt install -y qtbase5-dev libpqxx-dev libspdlog-dev`. |
| **Application log location** | Debug and audit logs | View runtime logs in the generated `logs/student_management.log` file. |

---
