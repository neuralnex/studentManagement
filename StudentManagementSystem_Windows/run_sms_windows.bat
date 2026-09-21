@echo off
title Student Management System Launcher
echo ========================================================
echo   Student Management System - Windows Launcher
echo ========================================================
echo.

:: Check if WSL is installed
where wsl >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] WSL (Windows Subsystem for Linux) was not found on this system.
    echo.
    echo To run this pre-built binary on Windows 10 or 11:
    echo 1. Open PowerShell as Administrator and run:
    echo      wsl --install
    echo 2. Restart your computer.
    echo 3. Run this script again.
    echo.
    echo Alternatively, you can build natively on Windows using Visual Studio:
    echo See README_WINDOWS.md for native MSVC build instructions.
    echo.
    pause
    exit /b 1
)

echo [INFO] Detected WSL environment.
echo [INFO] Launching Student Management System GUI...
echo.

:: Convert current Windows directory to WSL path and run
wsl -e bash -c "cd '$(wslpath -u '%~dp0')' && ./student_management_system.exe"

if %errorlevel% neq 0 (
    echo.
    echo [NOTICE] Application exited with code %errorlevel%.
    echo If libraries are missing in WSL, run:
    echo   wsl sudo apt update
    echo   wsl sudo apt install -y qtbase5-dev libpqxx-dev libspdlog-dev
    echo.
    pause
)
