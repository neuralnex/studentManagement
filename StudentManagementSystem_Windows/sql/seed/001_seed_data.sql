-- Student Management System - Seed Data
-- Seed: 001_seed_data.sql
-- Created: 2026-09-01
-- Description: Inserts baseline roles, an admin user, sample students,
--              a lecturer, a department, sample courses, and initial
--              enrollments so the system is immediately usable after
--              running the migrations.
--
-- Idempotent: every INSERT uses ON CONFLICT DO NOTHING, so this file
--             can be re-run safely without duplicating rows.
--
-- Password hash format used below:
--   $2b$12$<22-char-salt>$<64-char-digest>
--   The digest is a placeholder zeroed string; the application's own
--   bcrypt/sha256 logic will re-hash passwords on first change.
--   Placeholder hash: $2b$12$SeedSalt0000000000000$0000...0000 (64 zeros)

BEGIN;

-- ============================================================================
-- ROLES
-- ============================================================================
-- Ensure the fundamental roles exist before inserting users.

INSERT INTO roles (name, description, status)
VALUES
    ('Admin',    'System administrator with full access',   'ACTIVE'),
    ('Student',  'Enrolled student',                        'ACTIVE'),
    ('Lecturer', 'Teaching staff member',                   'ACTIVE'),
    ('Staff',    'Non-teaching administrative staff',       'ACTIVE')
ON CONFLICT (name) DO NOTHING;

-- ============================================================================
-- ADMIN USER
-- ============================================================================
-- Email    : admin@university.edu
-- Password : Admin@123456  (placeholder hash — change on first login)
-- Role     : Admin

INSERT INTO users (email, password_hash, first_name, last_name, role_id, status)
SELECT
    'admin@university.edu',
    '$2b$12$SeedSalt0000000000000$0000000000000000000000000000000000000000000000000000000000000000',
    'System',
    'Administrator',
    r.id,
    'ACTIVE'
FROM roles r
WHERE r.name = 'Admin'
ON CONFLICT (email) DO NOTHING;

-- ============================================================================
-- LECTURER USER
-- ============================================================================
-- Email    : dr.smith@university.edu
-- Password : placeholder hash
-- Role     : Lecturer

INSERT INTO users (email, password_hash, first_name, last_name, role_id, status)
SELECT
    'dr.smith@university.edu',
    '$2b$12$SeedSalt0000000000000$0000000000000000000000000000000000000000000000000000000000000000',
    'Alice',
    'Smith',
    r.id,
    'ACTIVE'
FROM roles r
WHERE r.name = 'Lecturer'
ON CONFLICT (email) DO NOTHING;

-- ============================================================================
-- STUDENT USERS  (first_name / last_name live in users, not students)
-- ============================================================================

INSERT INTO users (email, password_hash, first_name, last_name, role_id, status)
SELECT
    'john.doe@student.edu',
    '$2b$12$SeedSalt0000000000000$0000000000000000000000000000000000000000000000000000000000000000',
    'John',
    'Doe',
    r.id,
    'ACTIVE'
FROM roles r
WHERE r.name = 'Student'
ON CONFLICT (email) DO NOTHING;

INSERT INTO users (email, password_hash, first_name, last_name, role_id, status)
SELECT
    'jane.smith@student.edu',
    '$2b$12$SeedSalt0000000000000$0000000000000000000000000000000000000000000000000000000000000000',
    'Jane',
    'Smith',
    r.id,
    'ACTIVE'
FROM roles r
WHERE r.name = 'Student'
ON CONFLICT (email) DO NOTHING;

INSERT INTO users (email, password_hash, first_name, last_name, role_id, status)
SELECT
    'bob.johnson@student.edu',
    '$2b$12$SeedSalt0000000000000$0000000000000000000000000000000000000000000000000000000000000000',
    'Bob',
    'Johnson',
    r.id,
    'ACTIVE'
FROM roles r
WHERE r.name = 'Student'
ON CONFLICT (email) DO NOTHING;

-- ============================================================================
-- LECTURER RECORD
-- ============================================================================
-- Links the Dr. Smith user to the lecturers table.
-- department_id is set after the department is inserted (see below).

INSERT INTO lecturers (user_id, employee_id, status)
SELECT
    u.id,
    'EMP/2024/001',
    'ACTIVE'
FROM users u
WHERE u.email = 'dr.smith@university.edu'
ON CONFLICT (user_id) DO NOTHING;

-- ============================================================================
-- STUDENT RECORDS
-- ============================================================================
-- The students table does NOT have first_name / last_name (those are in users).
-- Names are retrieved via JOIN users ON students.user_id = users.id.

INSERT INTO students (user_id, matric_number, status)
SELECT u.id, 'STU/2024/001', 'ACTIVE'
FROM users u
WHERE u.email = 'john.doe@student.edu'
ON CONFLICT (matric_number) DO NOTHING;

INSERT INTO students (user_id, matric_number, status)
SELECT u.id, 'STU/2024/002', 'ACTIVE'
FROM users u
WHERE u.email = 'jane.smith@student.edu'
ON CONFLICT (matric_number) DO NOTHING;

INSERT INTO students (user_id, matric_number, status)
SELECT u.id, 'STU/2024/003', 'ACTIVE'
FROM users u
WHERE u.email = 'bob.johnson@student.edu'
ON CONFLICT (matric_number) DO NOTHING;

-- ============================================================================
-- DEPARTMENT
-- ============================================================================

INSERT INTO departments (name, code, status)
VALUES ('Computer Science', 'CS', 'ACTIVE')
ON CONFLICT (code) DO NOTHING;

-- Now that the department exists, link the lecturer to it
UPDATE lecturers
SET department_id = (SELECT id FROM departments WHERE code = 'CS')
WHERE user_id = (SELECT id FROM users WHERE email = 'dr.smith@university.edu')
  AND department_id IS NULL;

-- ============================================================================
-- COURSES
-- ============================================================================
-- lecturer_id is NULL for now; assign later via admin UI or another migration.
-- Semester values match the C++ Semester enum: FALL, SPRING, SUMMER.
-- enrolled_count is the canonical column added in migration 002.

INSERT INTO courses (code, title, description, credit_units, department_id, lecturer_id,
                     semester, year, capacity, current_enrollment, enrolled_count, status)
SELECT
    'CS101',
    'Introduction to Programming',
    'Fundamentals of programming using Python and C++.',
    3,
    d.id,
    NULL,
    'FALL',
    2026,
    30,
    0,
    0,
    'ACTIVE'
FROM departments d
WHERE d.code = 'CS'
ON CONFLICT (code) DO NOTHING;

INSERT INTO courses (code, title, description, credit_units, department_id, lecturer_id,
                     semester, year, capacity, current_enrollment, enrolled_count, status)
SELECT
    'CS201',
    'Data Structures',
    'Arrays, linked lists, stacks, queues, trees, and graphs.',
    3,
    d.id,
    NULL,
    'FALL',
    2026,
    25,
    0,
    0,
    'ACTIVE'
FROM departments d
WHERE d.code = 'CS'
ON CONFLICT (code) DO NOTHING;

INSERT INTO courses (code, title, description, credit_units, department_id, lecturer_id,
                     semester, year, capacity, current_enrollment, enrolled_count, status)
SELECT
    'CS301',
    'Database Systems',
    'Relational model, SQL, normalization, and transactions.',
    3,
    d.id,
    NULL,
    'SPRING',
    2026,
    20,
    0,
    0,
    'ACTIVE'
FROM departments d
WHERE d.code = 'CS'
ON CONFLICT (code) DO NOTHING;

-- ============================================================================
-- ENROLLMENTS
-- ============================================================================
-- Enroll STU/2024/001 (John Doe) in CS101 and CS201 with status ACTIVE.
-- semester is stored as the free-form string used by the C++ entity.

INSERT INTO enrollments
    (student_id, course_id, enrollment_date, enrolled_date,
     status, semester, year, credit_hours, prerequisites_met,
     grade_posted, grade, gpa,
     created_at, updated_at)
SELECT
    s.id,
    c.id,
    CURRENT_DATE,
    CURRENT_DATE,
    'ACTIVE',
    'FALL2026',
    2026,
    c.credit_units,
    TRUE,
    FALSE,
    0,
    0.0,
    NOW(),
    NOW()
FROM students s
JOIN users    u ON u.id  = s.user_id
JOIN courses  c ON c.code = 'CS101'
WHERE u.email = 'john.doe@student.edu'
ON CONFLICT (student_id, course_id) DO NOTHING;

INSERT INTO enrollments
    (student_id, course_id, enrollment_date, enrolled_date,
     status, semester, year, credit_hours, prerequisites_met,
     grade_posted, grade, gpa,
     created_at, updated_at)
SELECT
    s.id,
    c.id,
    CURRENT_DATE,
    CURRENT_DATE,
    'ACTIVE',
    'FALL2026',
    2026,
    c.credit_units,
    TRUE,
    FALSE,
    0,
    0.0,
    NOW(),
    NOW()
FROM students s
JOIN users    u ON u.id  = s.user_id
JOIN courses  c ON c.code = 'CS201'
WHERE u.email = 'john.doe@student.edu'
ON CONFLICT (student_id, course_id) DO NOTHING;

-- ============================================================================
-- Update enrolled_count on courses to reflect new enrollments
-- ============================================================================

UPDATE courses c
SET enrolled_count   = (
        SELECT COUNT(*)
        FROM   enrollments e
        WHERE  e.course_id = c.id
          AND  e.status IN ('PENDING', 'APPROVED', 'ACTIVE')
    ),
    current_enrollment = (
        SELECT COUNT(*)
        FROM   enrollments e
        WHERE  e.course_id = c.id
          AND  e.status IN ('PENDING', 'APPROVED', 'ACTIVE')
    )
WHERE c.code IN ('CS101', 'CS201', 'CS301');

COMMIT;
