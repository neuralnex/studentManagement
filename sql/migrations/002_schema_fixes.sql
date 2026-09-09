-- Student Management System - Schema Fixes
-- Migration: 002_schema_fixes.sql
-- Created: 2026-09-01
-- Description: Aligns enrollments and courses tables with C++ domain entities.
--              Fixes CHECK constraints, removes the redundant approval_status column,
--              adds missing columns referenced by PostgreSQLEnrollmentRepository,
--              and adds enrolled_count / year to courses.
--
-- Idempotent: safe to run multiple times.

BEGIN;

-- ============================================================================
-- ENROLLMENTS TABLE — status CHECK constraint
-- ============================================================================
-- The original constraint only allows ACTIVE, DROPPED, COMPLETED, INACTIVE.
-- The C++ EnrollmentStatus enum requires: PENDING, APPROVED, ACTIVE, COMPLETED,
-- DROPPED, WITHDRAWN, FAILED.

ALTER TABLE enrollments
    DROP CONSTRAINT IF EXISTS chk_enrollment_status;

ALTER TABLE enrollments
    ADD CONSTRAINT chk_enrollment_status
        CHECK (status IN ('PENDING', 'APPROVED', 'ACTIVE', 'COMPLETED', 'DROPPED', 'WITHDRAWN', 'FAILED'));

-- ============================================================================
-- ENROLLMENTS TABLE — remove approval_status (consolidate into status)
-- ============================================================================
-- The separate approval_status column (PENDING / APPROVED / REJECTED) is
-- redundant now that status itself carries PENDING and APPROVED states.
-- Drop the FK-dependent index and the column.

DROP INDEX IF EXISTS idx_enrollments_approval_status;

ALTER TABLE enrollments
    DROP CONSTRAINT IF EXISTS chk_approval_status;

ALTER TABLE enrollments
    DROP COLUMN IF EXISTS approval_status;

ALTER TABLE enrollments
    DROP COLUMN IF EXISTS approved_by;

ALTER TABLE enrollments
    DROP COLUMN IF EXISTS approval_date;

-- ============================================================================
-- ENROLLMENTS TABLE — add missing columns
-- ============================================================================
-- All columns referenced by PostgreSQLEnrollmentRepository SELECT/INSERT/UPDATE
-- queries that were absent from the original schema.

-- semester: free-form string like "FALL2026" (matches m_semester in Enrollment)
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS semester VARCHAR(20);

-- year: integer academic year (matches m_year in Enrollment)
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS year INTEGER;

-- grade: numerical grade 0–100 stored to 2 decimal places
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS grade NUMERIC(5,2) DEFAULT 0;

-- letter_grade: e.g. "A", "B+", "F"
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS letter_grade VARCHAR(2);

-- grade_posted: flag set true when a lecturer formally submits the grade
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS grade_posted BOOLEAN DEFAULT FALSE;

-- gpa: per-enrollment GPA contribution (e.g. 4.0, 3.7) stored for weighted
--      GPA calculations in calculateStudentGPA()
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS gpa NUMERIC(4,2) DEFAULT 0.0;

-- credit_hours: credits awarded on completion (mirrors courses.credit_units
--               but captured at enrolment time so it is stable if course changes)
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS credit_hours INTEGER DEFAULT 0;

-- prerequisites_met: checked before enrolment is activated
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS prerequisites_met BOOLEAN DEFAULT TRUE;

-- enrolled_date: the calendar date the enrolment record was created
--   (the original schema had enrollment_date NOT NULL; we keep that column and
--    add enrolled_date as a nullable alias used by the C++ entity's enrolledDate())
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS enrolled_date DATE;

-- Backfill enrolled_date from the existing enrollment_date where NULL
UPDATE enrollments
    SET enrolled_date = enrollment_date
    WHERE enrolled_date IS NULL AND enrollment_date IS NOT NULL;

-- completed_date: timestamp when a COMPLETED/FAILED status was recorded
ALTER TABLE enrollments
    ADD COLUMN IF NOT EXISTS completed_date TIMESTAMP WITH TIME ZONE;

-- ============================================================================
-- ENROLLMENTS TABLE — add useful index for semester+year queries
-- ============================================================================
CREATE INDEX IF NOT EXISTS idx_enrollments_semester_year
    ON enrollments(semester, year);

CREATE INDEX IF NOT EXISTS idx_enrollments_grade_posted
    ON enrollments(grade_posted);

-- ============================================================================
-- COURSES TABLE — semester CHECK constraint
-- ============================================================================
-- Original: ('FIRST', 'SECOND', 'FULL_YEAR')
-- C++ Semester enum: FALL, SPRING, SUMMER

ALTER TABLE courses
    DROP CONSTRAINT IF EXISTS chk_semester;

ALTER TABLE courses
    ADD CONSTRAINT chk_semester
        CHECK (semester IN ('FALL', 'SPRING', 'SUMMER'));

-- ============================================================================
-- COURSES TABLE — add year column
-- ============================================================================
-- The C++ Course entity (m_year) and ICourseRepository::findBySemesterAndYear()
-- both require a year column on courses.

ALTER TABLE courses
    ADD COLUMN IF NOT EXISTS year INTEGER;

-- ============================================================================
-- COURSES TABLE — add enrolled_count column
-- ============================================================================
-- The C++ Course entity uses enrolledCount() / setEnrolledCount(); the original
-- schema named this current_enrollment.  We add enrolled_count as the
-- canonical column and copy existing values so no data is lost.

ALTER TABLE courses
    ADD COLUMN IF NOT EXISTS enrolled_count INTEGER DEFAULT 0;

-- Backfill from current_enrollment for any pre-existing rows
UPDATE courses
    SET enrolled_count = current_enrollment
    WHERE enrolled_count = 0 AND current_enrollment > 0;

-- ============================================================================
-- COMMENTS
-- ============================================================================

COMMENT ON COLUMN enrollments.semester        IS 'Semester string, e.g. FALL2026 (matches C++ m_semester)';
COMMENT ON COLUMN enrollments.year            IS 'Academic year, e.g. 2026';
COMMENT ON COLUMN enrollments.grade           IS 'Numerical grade 0-100';
COMMENT ON COLUMN enrollments.letter_grade    IS 'Letter grade, e.g. A, B+, F';
COMMENT ON COLUMN enrollments.grade_posted    IS 'True once lecturer has formally submitted the grade';
COMMENT ON COLUMN enrollments.gpa             IS 'GPA points for this enrollment (used in weighted GPA calculation)';
COMMENT ON COLUMN enrollments.credit_hours    IS 'Credit hours captured at enrollment time';
COMMENT ON COLUMN enrollments.prerequisites_met IS 'Whether prerequisites were satisfied before enrollment';
COMMENT ON COLUMN enrollments.enrolled_date   IS 'Date the student was enrolled';
COMMENT ON COLUMN enrollments.completed_date  IS 'Timestamp when course was completed or failed';

COMMENT ON COLUMN courses.year          IS 'Academic year the course is offered, e.g. 2026';
COMMENT ON COLUMN courses.enrolled_count IS 'Current enrollment count (mirrors current_enrollment; canonical for C++ entity)';

COMMIT;
