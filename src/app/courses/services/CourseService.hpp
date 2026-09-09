#pragma once

#include "ICourseService.hpp"
#include <memory>

namespace sms::app::courses::services {

class CourseService : public ICourseService {
public:
    explicit CourseService(
        std::shared_ptr<ICourseRepository> courseRepo,
        std::shared_ptr<ICoursePrerequisiteRepository> prereqRepo
    );

    Course createCourse(
        const std::string& code,
        const std::string& title,
        int credits,
        int64_t deptId
    ) override;

    std::optional<Course> getCourse(int64_t courseId) const override;
    std::optional<Course> getCourseByCode(const std::string& code) const override;
    Course updateCourse(const Course& course) override;
    void assignLecturer(int64_t courseId, int64_t lecturerId) override;
    void setCourseCapacity(int64_t courseId, int capacity) override;
    void archiveCourse(int64_t courseId) override;
    void addPrerequisite(int64_t courseId, int64_t prereqCourseId) override;
    std::vector<CoursePrerequisite> getPrerequisites(int64_t courseId) const override;
    std::vector<Course> searchCourses(const std::string& searchTerm) const override;
    std::vector<Course> getActiveCourses(Semester semester, int year) const override;
    std::vector<Course> getCoursesByDepartment(int64_t deptId) const override;
    std::vector<Course> getCoursesByLecturer(int64_t lecturerId) const override;
    int64_t getTotalCourseCount() const override;

private:
    std::shared_ptr<ICourseRepository> m_courseRepo;
};

} // namespace sms::app::courses::services
