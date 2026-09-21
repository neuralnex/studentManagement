# Qt5 setup — must come before add_library
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(sms_core STATIC
    config/DatabaseConfig.cpp
    database/ConnectionPool.cpp
    utilities/PasswordHasher.cpp
    app/authentication/domain/Entities.hpp
    app/authentication/repositories/PostgreSQLUserRepository.cpp
    app/authentication/repositories/PostgreSQLRoleRepository.cpp
    app/authentication/repositories/PostgreSQLPermissionRepository.cpp
    app/authentication/services/AuthenticationService.cpp
    app/students/domain/Entities.hpp
    app/students/repositories/PostgreSQLStudentRepository.cpp
    app/students/services/StudentService.cpp
    app/courses/domain/Entities.hpp
    app/courses/repositories/PostgreSQLCourseRepository.cpp
    app/enrollments/domain/Entities.hpp
    app/enrollments/repositories/PostgreSQLEnrollmentRepository.cpp
    app/enrollments/services/EnrollmentService.cpp
    app/gui/LoginDialog.cpp
    app/gui/DashboardPage.cpp
    app/gui/StudentsPage.cpp
    app/gui/CoursesPage.cpp
    app/gui/EnrollmentsPage.cpp
    app/gui/MainWindow.cpp
    app/gui/Application.cpp
)

target_include_directories(sms_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${PostgreSQL_INCLUDE_DIRS}
    ${pqxx_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
)

target_link_libraries(sms_core PUBLIC
    PostgreSQL::PostgreSQL
    pqxx::pqxx
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    OpenSSL::Crypto
    Threads::Threads
    Qt5::Core
    Qt5::Widgets
)

target_compile_features(sms_core PUBLIC cxx_std_20)

# GUI executable (WIN32 suppresses the console window on Windows)
add_executable(student_management_system WIN32 main.cpp)
target_link_libraries(student_management_system PRIVATE sms_core)
target_compile_features(student_management_system PRIVATE cxx_std_20)

add_custom_command(TARGET student_management_system POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            $<TARGET_FILE:student_management_system>
            ${CMAKE_BINARY_DIR}/sms_app
    COMMENT "Creating convenience symlink build/sms_app"
)

if(NOT WIN32)
    add_custom_command(TARGET student_management_system POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                $<TARGET_FILE:student_management_system>
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/student_management_system.exe
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                $<TARGET_FILE:student_management_system>
                ${CMAKE_BINARY_DIR}/student_management_system.exe
        COMMENT "Creating student_management_system.exe symlinks"
    )
endif()
