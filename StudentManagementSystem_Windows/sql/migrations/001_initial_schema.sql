-- Student Management System - Initial Database Schema
-- Migration: 001_initial_schema.sql
-- Created: 2026-01-01
-- Description: Creates foundational tables for authentication and RBAC

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ============================================================================
-- PERMISSIONS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS permissions (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    description TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_permissions_name ON permissions(name);

COMMENT ON TABLE permissions IS 'System permissions that can be assigned to roles';
COMMENT ON COLUMN permissions.name IS 'Unique permission identifier (e.g., create_student, view_results)';

-- ============================================================================
-- ROLES TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS roles (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    description TEXT,
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_role_status CHECK (status IN ('ACTIVE', 'INACTIVE', 'ARCHIVED'))
);

CREATE INDEX idx_roles_name ON roles(name);
CREATE INDEX idx_roles_status ON roles(status);

COMMENT ON TABLE roles IS 'User roles defining sets of permissions';
COMMENT ON COLUMN roles.status IS 'ACTIVE, INACTIVE, or ARCHIVED';

-- ============================================================================
-- ROLE_PERMISSIONS JUNCTION TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS role_permissions (
    id BIGSERIAL PRIMARY KEY,
    role_id BIGINT NOT NULL,
    permission_id BIGINT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_role_permissions_role FOREIGN KEY (role_id) 
        REFERENCES roles(id) ON DELETE CASCADE,
    CONSTRAINT fk_role_permissions_permission FOREIGN KEY (permission_id) 
        REFERENCES permissions(id) ON DELETE CASCADE,
    CONSTRAINT unique_role_permission UNIQUE(role_id, permission_id)
);

CREATE INDEX idx_role_permissions_role_id ON role_permissions(role_id);
CREATE INDEX idx_role_permissions_permission_id ON role_permissions(permission_id);

COMMENT ON TABLE role_permissions IS 'Junction table mapping roles to permissions';

-- ============================================================================
-- USERS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    email VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    first_name VARCHAR(255),
    last_name VARCHAR(255),
    role_id BIGINT NOT NULL,
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    failed_login_attempts INTEGER DEFAULT 0,
    last_login_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_users_role FOREIGN KEY (role_id) 
        REFERENCES roles(id) ON DELETE RESTRICT,
    CONSTRAINT chk_user_status CHECK (status IN ('ACTIVE', 'INACTIVE', 'LOCKED', 'DELETED')),
    CONSTRAINT chk_email_format CHECK (email ~ '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$')
);

CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_role_id ON users(role_id);
CREATE INDEX idx_users_status ON users(status);
CREATE INDEX idx_users_created_at ON users(created_at);

COMMENT ON TABLE users IS 'System users with authentication credentials';
COMMENT ON COLUMN users.status IS 'ACTIVE, INACTIVE, LOCKED, or DELETED';
COMMENT ON COLUMN users.failed_login_attempts IS 'Count of consecutive failed login attempts';

-- ============================================================================
-- SESSIONS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS sessions (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    token VARCHAR(500) NOT NULL UNIQUE,
    ip_address INET,
    user_agent TEXT,
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_sessions_user FOREIGN KEY (user_id) 
        REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX idx_sessions_user_id ON sessions(user_id);
CREATE INDEX idx_sessions_token ON sessions(token);
CREATE INDEX idx_sessions_expires_at ON sessions(expires_at);

COMMENT ON TABLE sessions IS 'User session tokens for authentication';
COMMENT ON COLUMN sessions.ip_address IS 'IP address from which session was created';

-- ============================================================================
-- AUDIT_LOGS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS audit_logs (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT,
    action VARCHAR(255) NOT NULL,
    entity_type VARCHAR(255) NOT NULL,
    entity_id BIGINT,
    old_values JSONB,
    new_values JSONB,
    ip_address INET,
    user_agent TEXT,
    status VARCHAR(50) DEFAULT 'SUCCESS',
    error_message TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_audit_logs_user FOREIGN KEY (user_id) 
        REFERENCES users(id) ON DELETE SET NULL
);

CREATE INDEX idx_audit_logs_user_id ON audit_logs(user_id);
CREATE INDEX idx_audit_logs_entity_type ON audit_logs(entity_type);
CREATE INDEX idx_audit_logs_entity_id ON audit_logs(entity_id);
CREATE INDEX idx_audit_logs_action ON audit_logs(action);
CREATE INDEX idx_audit_logs_created_at ON audit_logs(created_at);

COMMENT ON TABLE audit_logs IS 'Complete audit trail of all system actions';
COMMENT ON COLUMN audit_logs.old_values IS 'Previous values before change (JSON)';
COMMENT ON COLUMN audit_logs.new_values IS 'New values after change (JSON)';

-- ============================================================================
-- STUDENTS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS students (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT UNIQUE,
    matric_number VARCHAR(50) NOT NULL UNIQUE,
    date_of_birth DATE,
    phone_number VARCHAR(20),
    emergency_contact_name VARCHAR(255),
    emergency_contact_phone VARCHAR(20),
    guardian_name VARCHAR(255),
    guardian_phone VARCHAR(20),
    guardian_email VARCHAR(255),
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    academic_standing VARCHAR(50),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_students_user FOREIGN KEY (user_id) 
        REFERENCES users(id) ON DELETE SET NULL,
    CONSTRAINT chk_student_status CHECK (status IN ('ACTIVE', 'SUSPENDED', 'GRADUATED', 'INACTIVE', 'DELETED')),
    CONSTRAINT chk_academic_standing CHECK (academic_standing IN ('GOOD', 'PROBATION', 'WARNING', NULL))
);

CREATE INDEX idx_students_user_id ON students(user_id);
CREATE INDEX idx_students_matric_number ON students(matric_number);
CREATE INDEX idx_students_status ON students(status);

COMMENT ON TABLE students IS 'Student records';
COMMENT ON COLUMN students.matric_number IS 'Unique student identification number';

-- ============================================================================
-- LECTURERS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS lecturers (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT UNIQUE NOT NULL,
    employee_id VARCHAR(50) NOT NULL UNIQUE,
    department_id BIGINT,
    phone_number VARCHAR(20),
    office_location VARCHAR(255),
    qualification VARCHAR(255),
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_lecturers_user FOREIGN KEY (user_id) 
        REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT chk_lecturer_status CHECK (status IN ('ACTIVE', 'INACTIVE', 'RETIRED', 'DELETED'))
);

CREATE INDEX idx_lecturers_user_id ON lecturers(user_id);
CREATE INDEX idx_lecturers_employee_id ON lecturers(employee_id);
CREATE INDEX idx_lecturers_status ON lecturers(status);

COMMENT ON TABLE lecturers IS 'Lecturer/Staff records';

-- ============================================================================
-- DEPARTMENTS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS departments (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    code VARCHAR(50) NOT NULL UNIQUE,
    hod_id BIGINT,
    phone_number VARCHAR(20),
    email VARCHAR(255),
    building_location VARCHAR(255),
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_departments_hod FOREIGN KEY (hod_id) 
        REFERENCES lecturers(id) ON DELETE SET NULL,
    CONSTRAINT chk_department_status CHECK (status IN ('ACTIVE', 'INACTIVE', 'ARCHIVED'))
);

CREATE INDEX idx_departments_name ON departments(name);
CREATE INDEX idx_departments_code ON departments(code);
CREATE INDEX idx_departments_hod_id ON departments(hod_id);

COMMENT ON TABLE departments IS 'Academic departments';
COMMENT ON COLUMN departments.hod_id IS 'Head of Department (Lecturer ID)';

-- Update lecturer department_id foreign key now that departments table exists
ALTER TABLE lecturers 
ADD CONSTRAINT fk_lecturers_department FOREIGN KEY (department_id) 
REFERENCES departments(id) ON DELETE SET NULL;

-- ============================================================================
-- FACULTIES TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS faculties (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    dean_id BIGINT,
    phone_number VARCHAR(20),
    email VARCHAR(255),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_faculties_dean FOREIGN KEY (dean_id) 
        REFERENCES lecturers(id) ON DELETE SET NULL
);

CREATE INDEX idx_faculties_name ON faculties(name);
CREATE INDEX idx_faculties_dean_id ON faculties(dean_id);

COMMENT ON TABLE faculties IS 'Academic faculties (grouping of departments)';

-- ============================================================================
-- COURSES TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS courses (
    id BIGSERIAL PRIMARY KEY,
    code VARCHAR(50) NOT NULL UNIQUE,
    title VARCHAR(255) NOT NULL,
    description TEXT,
    credit_units INTEGER NOT NULL,
    department_id BIGINT NOT NULL,
    lecturer_id BIGINT,
    semester VARCHAR(50) NOT NULL,
    year INTEGER NOT NULL,
    location VARCHAR(255),
    capacity INTEGER,
    current_enrollment INTEGER DEFAULT 0,
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_courses_department FOREIGN KEY (department_id) 
        REFERENCES departments(id) ON DELETE RESTRICT,
    CONSTRAINT fk_courses_lecturer FOREIGN KEY (lecturer_id) 
        REFERENCES lecturers(id) ON DELETE SET NULL,
    CONSTRAINT chk_course_status CHECK (status IN ('ACTIVE', 'INACTIVE', 'ARCHIVED')),
    CONSTRAINT chk_semester CHECK (semester IN ('FIRST', 'SECOND', 'FULL_YEAR')),
    CONSTRAINT chk_capacity CHECK (capacity IS NULL OR capacity > 0),
    CONSTRAINT chk_credit_units CHECK (credit_units > 0)
);

CREATE INDEX idx_courses_code ON courses(code);
CREATE INDEX idx_courses_department_id ON courses(department_id);
CREATE INDEX idx_courses_lecturer_id ON courses(lecturer_id);
CREATE INDEX idx_courses_status ON courses(status);

COMMENT ON TABLE courses IS 'Course offerings';
COMMENT ON COLUMN courses.credit_units IS 'Credit hours for the course';

-- ============================================================================
-- COURSE_PREREQUISITES TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS course_prerequisites (
    id BIGSERIAL PRIMARY KEY,
    course_id BIGINT NOT NULL,
    prerequisite_course_id BIGINT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_course_prerequisites_course FOREIGN KEY (course_id) 
        REFERENCES courses(id) ON DELETE CASCADE,
    CONSTRAINT fk_course_prerequisites_prereq FOREIGN KEY (prerequisite_course_id) 
        REFERENCES courses(id) ON DELETE CASCADE,
    CONSTRAINT unique_prerequisite UNIQUE(course_id, prerequisite_course_id),
    CONSTRAINT chk_no_self_prerequisite CHECK (course_id != prerequisite_course_id)
);

CREATE INDEX idx_course_prerequisites_course_id ON course_prerequisites(course_id);
CREATE INDEX idx_course_prerequisites_prerequisite_id ON course_prerequisites(prerequisite_course_id);

COMMENT ON TABLE course_prerequisites IS 'Course prerequisite requirements';

-- ============================================================================
-- ENROLLMENTS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS enrollments (
    id BIGSERIAL PRIMARY KEY,
    student_id BIGINT NOT NULL,
    course_id BIGINT NOT NULL,
    enrollment_date DATE NOT NULL,
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    approval_status VARCHAR(50) DEFAULT 'PENDING',
    approved_by BIGINT,
    approval_date TIMESTAMP WITH TIME ZONE,
    dropped_at TIMESTAMP WITH TIME ZONE,
    semester VARCHAR(50),
    year INTEGER,
    grade NUMERIC(5,2),
    grade_posted BOOLEAN DEFAULT FALSE,
    letter_grade VARCHAR(2),
    gpa NUMERIC(3,2),
    credit_hours INTEGER,
    prerequisites_met BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_enrollments_student FOREIGN KEY (student_id)
        REFERENCES students(id) ON DELETE CASCADE,
    CONSTRAINT fk_enrollments_course FOREIGN KEY (course_id)
        REFERENCES courses(id) ON DELETE RESTRICT,
    CONSTRAINT fk_enrollments_approver FOREIGN KEY (approved_by)
        REFERENCES users(id) ON DELETE SET NULL,
    CONSTRAINT chk_enrollment_status CHECK (status IN ('ACTIVE', 'DROPPED', 'COMPLETED', 'INACTIVE')),
    CONSTRAINT chk_approval_status CHECK (approval_status IN ('PENDING', 'APPROVED', 'REJECTED')),
    CONSTRAINT unique_enrollment UNIQUE(student_id, course_id)
);

CREATE INDEX idx_enrollments_student_id ON enrollments(student_id);
CREATE INDEX idx_enrollments_course_id ON enrollments(course_id);
CREATE INDEX idx_enrollments_status ON enrollments(status);
CREATE INDEX idx_enrollments_approval_status ON enrollments(approval_status);

COMMENT ON TABLE enrollments IS 'Student course enrollments';

-- ============================================================================
-- RESULTS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS results (
    id BIGSERIAL PRIMARY KEY,
    enrollment_id BIGINT NOT NULL,
    student_id BIGINT NOT NULL,
    course_id BIGINT NOT NULL,
    continuous_assessment NUMERIC(5,2),
    exam_score NUMERIC(5,2),
    total_score NUMERIC(5,2),
    grade VARCHAR(2),
    grade_point NUMERIC(3,2),
    status VARCHAR(50) NOT NULL DEFAULT 'DRAFT',
    submitted_by BIGINT,
    submission_date TIMESTAMP WITH TIME ZONE,
    locked_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_results_enrollment FOREIGN KEY (enrollment_id) 
        REFERENCES enrollments(id) ON DELETE CASCADE,
    CONSTRAINT fk_results_student FOREIGN KEY (student_id) 
        REFERENCES students(id) ON DELETE CASCADE,
    CONSTRAINT fk_results_course FOREIGN KEY (course_id) 
        REFERENCES courses(id) ON DELETE RESTRICT,
    CONSTRAINT fk_results_submitted_by FOREIGN KEY (submitted_by) 
        REFERENCES users(id) ON DELETE SET NULL,
    CONSTRAINT chk_result_status CHECK (status IN ('DRAFT', 'SUBMITTED', 'APPROVED', 'LOCKED', 'REJECTED')),
    CONSTRAINT chk_scores CHECK (
        (continuous_assessment IS NULL OR continuous_assessment >= 0) AND
        (exam_score IS NULL OR exam_score >= 0) AND
        (total_score IS NULL OR total_score >= 0)
    )
);

CREATE INDEX idx_results_student_id ON results(student_id);
CREATE INDEX idx_results_course_id ON results(course_id);
CREATE INDEX idx_results_enrollment_id ON results(enrollment_id);
CREATE INDEX idx_results_status ON results(status);
CREATE INDEX idx_results_grade ON results(grade);

COMMENT ON TABLE results IS 'Student course results and grades';

-- ============================================================================
-- ATTENDANCE TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS attendance (
    id BIGSERIAL PRIMARY KEY,
    student_id BIGINT NOT NULL,
    course_id BIGINT NOT NULL,
    session_date DATE NOT NULL,
    status VARCHAR(50) NOT NULL,
    recorded_by BIGINT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_attendance_student FOREIGN KEY (student_id) 
        REFERENCES students(id) ON DELETE CASCADE,
    CONSTRAINT fk_attendance_course FOREIGN KEY (course_id) 
        REFERENCES courses(id) ON DELETE RESTRICT,
    CONSTRAINT fk_attendance_recorded_by FOREIGN KEY (recorded_by) 
        REFERENCES users(id) ON DELETE SET NULL,
    CONSTRAINT chk_attendance_status CHECK (status IN ('PRESENT', 'ABSENT', 'EXCUSED')),
    CONSTRAINT unique_attendance UNIQUE(student_id, course_id, session_date)
);

CREATE INDEX idx_attendance_student_id ON attendance(student_id);
CREATE INDEX idx_attendance_course_id ON attendance(course_id);
CREATE INDEX idx_attendance_session_date ON attendance(session_date);

COMMENT ON TABLE attendance IS 'Course attendance records';

-- ============================================================================
-- PAYMENTS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS payments (
    id BIGSERIAL PRIMARY KEY,
    student_id BIGINT NOT NULL,
    amount NUMERIC(12,2) NOT NULL,
    payment_type VARCHAR(50) NOT NULL,
    payment_date DATE,
    reference_number VARCHAR(255) UNIQUE,
    status VARCHAR(50) NOT NULL DEFAULT 'PENDING',
    payment_method VARCHAR(50),
    processed_by BIGINT,
    notes TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_payments_student FOREIGN KEY (student_id) 
        REFERENCES students(id) ON DELETE CASCADE,
    CONSTRAINT fk_payments_processed_by FOREIGN KEY (processed_by) 
        REFERENCES users(id) ON DELETE SET NULL,
    CONSTRAINT chk_payment_status CHECK (status IN ('PENDING', 'COMPLETED', 'FAILED', 'CANCELLED')),
    CONSTRAINT chk_payment_type CHECK (payment_type IN ('TUITION', 'ACCOMMODATION', 'LIBRARY', 'OTHER')),
    CONSTRAINT chk_amount CHECK (amount > 0)
);

CREATE INDEX idx_payments_student_id ON payments(student_id);
CREATE INDEX idx_payments_payment_date ON payments(payment_date);
CREATE INDEX idx_payments_status ON payments(status);

COMMENT ON TABLE payments IS 'Student fee payments';

-- ============================================================================
-- HOSTELS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS hostels (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    location VARCHAR(255),
    gender_type VARCHAR(50),
    total_capacity INTEGER NOT NULL,
    current_occupancy INTEGER DEFAULT 0,
    status VARCHAR(50) NOT NULL DEFAULT 'ACTIVE',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_hostel_status CHECK (status IN ('ACTIVE', 'MAINTENANCE', 'CLOSED')),
    CONSTRAINT chk_gender_type CHECK (gender_type IN ('MALE', 'FEMALE', 'MIXED', NULL)),
    CONSTRAINT chk_capacity CHECK (total_capacity > 0)
);

CREATE INDEX idx_hostels_name ON hostels(name);
CREATE INDEX idx_hostels_status ON hostels(status);

COMMENT ON TABLE hostels IS 'Hostel/Dormitory facilities';

-- ============================================================================
-- ROOMS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS rooms (
    id BIGSERIAL PRIMARY KEY,
    hostel_id BIGINT NOT NULL,
    room_number VARCHAR(50) NOT NULL,
    capacity INTEGER NOT NULL,
    current_occupancy INTEGER DEFAULT 0,
    status VARCHAR(50) NOT NULL DEFAULT 'AVAILABLE',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_rooms_hostel FOREIGN KEY (hostel_id) 
        REFERENCES hostels(id) ON DELETE CASCADE,
    CONSTRAINT unique_room_number UNIQUE(hostel_id, room_number),
    CONSTRAINT chk_room_status CHECK (status IN ('AVAILABLE', 'OCCUPIED', 'MAINTENANCE', 'CLOSED')),
    CONSTRAINT chk_room_capacity CHECK (capacity > 0)
);

CREATE INDEX idx_rooms_hostel_id ON rooms(hostel_id);
CREATE INDEX idx_rooms_status ON rooms(status);

COMMENT ON TABLE rooms IS 'Hostel rooms';

-- ============================================================================
-- BOOKS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS books (
    id BIGSERIAL PRIMARY KEY,
    isbn VARCHAR(20) UNIQUE,
    title VARCHAR(255) NOT NULL,
    author VARCHAR(255),
    publisher VARCHAR(255),
    publication_year INTEGER,
    total_copies INTEGER NOT NULL DEFAULT 1,
    available_copies INTEGER NOT NULL DEFAULT 1,
    category VARCHAR(100),
    status VARCHAR(50) NOT NULL DEFAULT 'AVAILABLE',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_book_copies CHECK (total_copies > 0),
    CONSTRAINT chk_book_available CHECK (available_copies >= 0 AND available_copies <= total_copies),
    CONSTRAINT chk_book_status CHECK (status IN ('AVAILABLE', 'OUT_OF_STOCK', 'DISCONTINUED'))
);

CREATE INDEX idx_books_isbn ON books(isbn);
CREATE INDEX idx_books_title ON books(title);
CREATE INDEX idx_books_category ON books(category);

COMMENT ON TABLE books IS 'Library book catalog';

-- ============================================================================
-- BORROWED_BOOKS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS borrowed_books (
    id BIGSERIAL PRIMARY KEY,
    student_id BIGINT NOT NULL,
    book_id BIGINT NOT NULL,
    borrow_date DATE NOT NULL,
    due_date DATE NOT NULL,
    return_date DATE,
    fine_amount NUMERIC(8,2) DEFAULT 0,
    status VARCHAR(50) NOT NULL DEFAULT 'BORROWED',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_borrowed_books_student FOREIGN KEY (student_id) 
        REFERENCES students(id) ON DELETE CASCADE,
    CONSTRAINT fk_borrowed_books_book FOREIGN KEY (book_id) 
        REFERENCES books(id) ON DELETE RESTRICT,
    CONSTRAINT chk_borrowed_status CHECK (status IN ('BORROWED', 'RETURNED', 'LOST', 'OVERDUE')),
    CONSTRAINT chk_fine_amount CHECK (fine_amount >= 0)
);

CREATE INDEX idx_borrowed_books_student_id ON borrowed_books(student_id);
CREATE INDEX idx_borrowed_books_book_id ON borrowed_books(book_id);
CREATE INDEX idx_borrowed_books_status ON borrowed_books(status);
CREATE INDEX idx_borrowed_books_due_date ON borrowed_books(due_date);

COMMENT ON TABLE borrowed_books IS 'Book borrowing records';

-- ============================================================================
-- NOTIFICATIONS TABLE
-- ============================================================================
CREATE TABLE IF NOT EXISTS notifications (
    id BIGSERIAL PRIMARY KEY,
    recipient_id BIGINT NOT NULL,
    title VARCHAR(255) NOT NULL,
    message TEXT NOT NULL,
    notification_type VARCHAR(50),
    related_entity_type VARCHAR(255),
    related_entity_id BIGINT,
    is_read BOOLEAN DEFAULT FALSE,
    read_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_notifications_recipient FOREIGN KEY (recipient_id) 
        REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX idx_notifications_recipient_id ON notifications(recipient_id);
CREATE INDEX idx_notifications_is_read ON notifications(is_read);
CREATE INDEX idx_notifications_created_at ON notifications(created_at);

COMMENT ON TABLE notifications IS 'User notifications';

-- ============================================================================
-- Create initial roles and permissions
-- ============================================================================

INSERT INTO permissions (name, description) VALUES
    ('create_student', 'Create new student record'),
    ('read_student', 'View student records'),
    ('update_student', 'Update student information'),
    ('delete_student', 'Delete student records'),
    ('manage_courses', 'Manage course offerings'),
    ('manage_enrollments', 'Manage student enrollments'),
    ('manage_results', 'Manage course results'),
    ('view_results', 'View course results'),
    ('manage_departments', 'Manage departments'),
    ('manage_users', 'Manage system users'),
    ('manage_roles', 'Manage roles and permissions'),
    ('view_reports', 'View system reports'),
    ('manage_payments', 'Manage student payments'),
    ('view_audit_logs', 'View audit logs');

INSERT INTO roles (name, description, status) VALUES
    ('Administrator', 'Full system access', 'ACTIVE'),
    ('Registrar', 'Registration and enrollment management', 'ACTIVE'),
    ('Lecturer', 'Teaching and grading', 'ACTIVE'),
    ('Student', 'Student self-service access', 'ACTIVE'),
    ('Department Officer', 'Department-level operations', 'ACTIVE'),
    ('Finance Officer', 'Financial operations', 'ACTIVE');

-- Assign all permissions to Administrator
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p WHERE r.name = 'Administrator';

-- Assign Registrar permissions
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p 
WHERE r.name = 'Registrar' AND p.name IN (
    'create_student', 'read_student', 'update_student', 'manage_enrollments', 
    'view_results', 'view_reports'
);

-- Assign Lecturer permissions
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p 
WHERE r.name = 'Lecturer' AND p.name IN (
    'read_student', 'manage_results', 'view_results'
);

-- Assign Student permissions
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p 
WHERE r.name = 'Student' AND p.name IN (
    'read_student', 'view_results'
);

-- ============================================================================
-- Create function to update updated_at timestamp
-- ============================================================================
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Create triggers for updated_at columns
CREATE TRIGGER update_users_updated_at BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_roles_updated_at BEFORE UPDATE ON roles
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_students_updated_at BEFORE UPDATE ON students
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_courses_updated_at BEFORE UPDATE ON courses
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_enrollments_updated_at BEFORE UPDATE ON enrollments
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_results_updated_at BEFORE UPDATE ON results
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_payments_updated_at BEFORE UPDATE ON payments
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_hostels_updated_at BEFORE UPDATE ON hostels
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_rooms_updated_at BEFORE UPDATE ON rooms
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_books_updated_at BEFORE UPDATE ON books
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_borrowed_books_updated_at BEFORE UPDATE ON borrowed_books
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();
