#include "CourseService.hpp"
#include <stdexcept>
#include <algorithm>

namespace sms::app::courses::services {

CourseService::CourseService(
    std::shared_ptr<ICourseRepository> courseRepo,
    std::shared_ptr<ICoursePrerequisiteRepository> prereqRepo
) : m_courseRepo(courseRepo), m_prereqRepo(prereqRepo) {}

Course CourseService::createCourse(
    const std::string& code,
    const std::string& title,
    int credits,
    int64_t deptId
) {
    if (m_courseRepo->codeExists(code)) {
        throw std::runtime_error("Course code already exists: " + code);
    }

    Course course;
    course.setCode(code);
    course.setTitle(title);
    course.setCreditUnits(credits);
    course.setDepartmentId(deptId);
    course.setStatus(CourseStatus::DRAFT);

    return m_courseRepo->save(course);
}

std::optional<Course> CourseService::getCourse(int64_t courseId) const {
    return m_courseRepo->findById(courseId);
}

std::optional<Course> CourseService::getCourseByCode(const std::string& code) const {
    return m_courseRepo->findByCode(code);
}

Course CourseService::updateCourse(const Course& course) {
    return m_courseRepo->save(course);
}

void CourseService::assignLecturer(int64_t courseId, int64_t lecturerId) {
    auto course = m_courseRepo->findById(courseId);
    if (!course) {
        throw std::runtime_error("Course not found: " + std::to_string(courseId));
    }

    course->setLecturerId(lecturerId);
    m_courseRepo->save(*course);
}

void CourseService::setCourseCapacity(int64_t courseId, int capacity) {
    auto course = m_courseRepo->findById(courseId);
    if (!course) {
        throw std::runtime_error("Course not found: " + std::to_string(courseId));
    }

    course->setCapacity(capacity);
    m_courseRepo->save(*course);
}

void CourseService::archiveCourse(int64_t courseId) {
    auto course = m_courseRepo->findById(courseId);
    if (!course) {
        throw std::runtime_error("Course not found: " + std::to_string(courseId));
    }

    course->setStatus(CourseStatus::ARCHIVED);
    m_courseRepo->save(*course);
}

void CourseService::addPrerequisite(int64_t courseId, int64_t prereqCourseId) {
    if (courseId == prereqCourseId) {
        throw std::runtime_error("A course cannot be its own prerequisite");
    }

    if (!m_courseRepo->findById(courseId)) {
        throw std::runtime_error("Course not found: " + std::to_string(courseId));
    }

    if (!m_courseRepo->findById(prereqCourseId)) {
        throw std::runtime_error("Prerequisite course not found: " + std::to_string(prereqCourseId));
    }

    m_prereqRepo->addPrerequisite(courseId, prereqCourseId);
}

std::vector<CoursePrerequisite> CourseService::getPrerequisites(int64_t courseId) const {
    return m_prereqRepo->getPrerequisites(courseId);
}

std::vector<Course> CourseService::searchCourses(const std::string& searchTerm) const {
    return m_courseRepo->search(searchTerm);
}

std::vector<Course> CourseService::getActiveCourses(Semester semester, int year) const {
    return m_courseRepo->findBySemester(semester, year);
}

std::vector<Course> CourseService::getCoursesByDepartment(int64_t deptId) const {
    return m_courseRepo->findByDepartment(deptId);
}

std::vector<Course> CourseService::getCoursesByLecturer(int64_t lecturerId) const {
    return m_courseRepo->findByLecturer(lecturerId);
}

int64_t CourseService::getTotalCourseCount() const {
    return m_courseRepo->count();
}

} // namespace sms::app::courses::services
