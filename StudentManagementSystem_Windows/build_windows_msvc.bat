@echo off
title Build Student Management System (Windows Native MSVC)
echo ========================================================
echo   Build Student Management System (Native Windows MSVC)
echo ========================================================
echo.

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake is not installed or not in PATH.
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

if not defined VCPKG_ROOT (
    if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set VCPKG_ROOT=C:\vcpkg
    ) else (
        echo [WARNING] VCPKG_ROOT is not set.
        echo If using vcpkg, install dependencies with:
        echo   vcpkg install libpqxx spdlog nlohmann-json openssl qt5-base:x64-windows
        echo and pass -DCMAKE_TOOLCHAIN_FILE=path\to\vcpkg.cmake
    )
)

echo Configuring build with CMake...
if defined VCPKG_ROOT (
    cmake -B build_win -S . -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
) else (
    cmake -B build_win -S .
)

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo.
echo Compiling Release build...
cmake --build build_win --config Release

if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ========================================================
echo   BUILD SUCCESSFUL!
echo   Executable created at: build_win\bin\Release\student_management_system.exe
echo ========================================================
pause
