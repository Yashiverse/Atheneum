@echo off
echo ========================================================
echo Compiling Library Management System C Core Engine...
echo ========================================================
gcc -Wall -Wextra -O2 library_core.c -o library_core.exe
if %ERRORLEVEL% EQU 0 (
    echo [+] Compilation successful: library_core.exe
) else (
    echo [!] Compilation failed! Check error messages above.
)
