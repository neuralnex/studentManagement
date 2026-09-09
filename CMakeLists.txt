cmake_minimum_required(VERSION 3.20)
project(StudentManagementSystem 
    VERSION 1.0.0
    DESCRIPTION "Enterprise Student Database Management System"
    LANGUAGES CXX
)

# Local CMake modules (e.g. Findnlohmann_json.cmake for Debian's header-only
# nlohmann-json3-dev package, which ships no nlohmann_jsonConfig.cmake).
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Enable compile commands export for IDE IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(PostgreSQL REQUIRED)
find_package(pqxx REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED MODULE)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

# Third-party libraries
find_package(Threads REQUIRED)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# ─── Install target ─────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS student_management_system
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${CMAKE_SOURCE_DIR}/sql/migrations/001_initial_schema.sql
    DESTINATION ${CMAKE_INSTALL_DATADIR}/student-management-system/migrations
)
install(FILES ${CMAKE_SOURCE_DIR}/sql/migrations/002_schema_fixes.sql
    DESTINATION ${CMAKE_INSTALL_DATADIR}/student-management-system/migrations
)
install(FILES ${CMAKE_SOURCE_DIR}/sql/seed/001_seed_data.sql
    DESTINATION ${CMAKE_INSTALL_DATADIR}/student-management-system/seed
)

# ─── CPack / .deb packaging ─────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "student-management-system")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "University of Excellence")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Enterprise Student Database Management System")
set(CPACK_PACKAGE_DESCRIPTION "A production-grade student management system for universities.
Manages students, courses, enrollments, and results.")
set(CPACK_PACKAGE_CONTACT "admin@university.edu")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# .deb specific
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Leo Ajawara <admin@university.edu>")
set(CPACK_DEBIAN_PACKAGE_SECTION "education")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libpq5, libpqxx-dev, libssl3, libncurses6")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "postgresql")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "postgresql-client")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/university/sms")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/debian/postinst"
    "${CMAKE_SOURCE_DIR}/debian/prerm"
)

# .zip for Windows (the exe + bundled note)
set(CPACK_GENERATOR "DEB;ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

include(CPack)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "PostgreSQL Include: ${PostgreSQL_INCLUDE_DIRS}")
message(STATUS "PostgreSQL Library: ${PostgreSQL_LIBRARIES}")
