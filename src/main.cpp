/**
 * @file main.cpp
 * @brief Entry point for the Student Management System (Qt5 GUI)
 *
 * Startup sequence:
 *   1. Load .env from the current directory (if present)
 *   2. Initialise spdlog (console + file sink in logs/)
 *   3. Load DB config from environment variables
 *   4. Initialise ConnectionPool
 *   5. Construct all six PostgreSQL repositories
 *   6. Construct AuthenticationService, StudentService, EnrollmentService
 *   7. Hand off to Application::run() which owns the Qt event loop
 */

#include <QApplication>  // needed for int argc, char* argv[]

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "config/DatabaseConfig.hpp"
#include "database/ConnectionPool.hpp"
#include "app/authentication/repositories/PostgreSQLUserRepository.hpp"
#include "app/authentication/repositories/PostgreSQLRoleRepository.hpp"
#include "app/authentication/repositories/PostgreSQLPermissionRepository.hpp"
#include "app/authentication/services/AuthenticationService.hpp"
#include "app/students/repositories/PostgreSQLStudentRepository.hpp"
#include "app/students/services/StudentService.hpp"
#include "app/courses/repositories/PostgreSQLCourseRepository.hpp"
#include "app/enrollments/repositories/PostgreSQLEnrollmentRepository.hpp"
#include "app/enrollments/services/EnrollmentService.hpp"
#include "app/gui/Application.hpp"

namespace {

// ─── Logging ─────────────────────────────────────────────────────────────────

void initializeLogging() {
    std::filesystem::create_directories("logs");

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "logs/student_management.log", /*truncate=*/false
    );
    fileSink->set_level(spdlog::level::debug);

    auto logger = std::make_shared<spdlog::logger>(
        "sms",
        spdlog::sinks_init_list{consoleSink, fileSink}
    );
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::err);

    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

    spdlog::info("Logging initialised — file: logs/student_management.log");
}

// ─── Database ─────────────────────────────────────────────────────────────────

std::shared_ptr<sms::database::ConnectionPool> initializeDatabase() {
    using sms::config::DatabaseConfig;
    using sms::database::ConnectionPool;

    auto config = DatabaseConfig::loadFromEnvironment();
    if (!config) {
        throw std::runtime_error(
            "Failed to load database configuration from environment"
        );
    }

    auto pool = std::make_shared<ConnectionPool>(std::move(config));

    if (!pool->isHealthy()) {
        throw std::runtime_error(
            "Database connection pool is unhealthy — check DB_HOST, DB_PORT, "
            "DB_NAME, DB_USER, DB_PASSWORD environment variables"
        );
    }

    spdlog::info("Connection pool ready ({} connections)", pool->getMaxSize());
    return pool;
}

} // anonymous namespace

// ─── Entry point ─────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    try {
        // 1. Load .env before anything else so env vars are available
        //    for logging config too.
        sms::config::DatabaseConfig::loadDotEnv();

        // 2. Logging
        initializeLogging();
        spdlog::info("Student Management System v1.0.0");

        // 3. Database
        auto pool = initializeDatabase();

        // 4. Repositories
        using namespace sms::app::authentication::repositories;
        using namespace sms::app::students::repositories;
        using namespace sms::app::courses::repositories;
        using namespace sms::app::enrollments::repositories;

        auto userRepo    = std::make_shared<PostgreSQLUserRepository>(pool);
        auto roleRepo    = std::make_shared<PostgreSQLRoleRepository>(pool);
        auto permRepo    = std::make_shared<PostgreSQLPermissionRepository>(pool);
        auto studentRepo = std::make_shared<PostgreSQLStudentRepository>(pool);
        auto courseRepo  = std::make_shared<PostgreSQLCourseRepository>(pool);
        auto enrollRepo  = std::make_shared<PostgreSQLEnrollmentRepository>(pool);

        spdlog::debug("All repositories constructed");

        // 5. Services
        using namespace sms::app::authentication::services;
        using namespace sms::app::students::services;
        using namespace sms::app::enrollments::services;

        auto authService = std::make_shared<AuthenticationService>(
            userRepo, roleRepo, permRepo
        );
        auto studentService = std::make_shared<StudentService>(studentRepo);
        auto enrollmentService = std::make_shared<EnrollmentService>(
            enrollRepo, courseRepo, studentRepo
        );

        spdlog::debug("All services constructed");

        // 6. GUI application — owns the Qt event loop
        sms::app::gui::Application guiApp(
            authService, studentService, courseRepo, enrollmentService
        );

        spdlog::info("Launching GUI");
        return guiApp.run(argc, argv);

    } catch (const std::exception& ex) {
        // Logger may not be up yet if the exception is very early
        std::cerr << "[FATAL] " << ex.what() << '\n';
        try {
            spdlog::critical("Fatal startup error: {}", ex.what());
        } catch (...) {}
        return EXIT_FAILURE;
    }
}
