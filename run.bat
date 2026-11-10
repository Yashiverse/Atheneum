@echo off
title Athenaeum Library Management System
echo ===================================================================
echo     ATHENAEUM LIBRARY MANAGEMENT SYSTEM (Node.js & C Engine)
echo ===================================================================
echo.

:: 1. Compile C Core Engine if not present
if not exist "backend\library_core.exe" (
    echo [*] Compiling C core backend with gcc...
    gcc -Wall -Wextra -O2 backend\library_core.c -o backend\library_core.exe
    if %ERRORLEVEL% NEQ 0 (
        echo [!] GCC compilation failed. Please ensure gcc is installed.
        pause
        exit /b %ERRORLEVEL%
    )
    echo [+] C backend compiled successfully.
)

:: 2. Launch web server using Node.js
node --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Node.js is not installed or not found in PATH.
    echo [!] Please install Node.js from https://nodejs.org
    pause
    exit /b 1
)

echo [*] Starting web server using Node.js...
echo [*] Opening interface at: http://localhost:8080
start http://localhost:8080
node server.js
