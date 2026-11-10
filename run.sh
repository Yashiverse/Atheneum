#!/usr/bin/env bash
echo "==================================================================="
echo "    ATHENAEUM LIBRARY MANAGEMENT SYSTEM (Node.js & C Engine)"
echo "==================================================================="
echo ""

# 1. Compile C Core Engine if not present
if [ ! -f "backend/library_core" ] && [ ! -f "backend/library_core.exe" ]; then
    echo "[*] Compiling C core backend with gcc..."
    gcc -Wall -Wextra -O2 backend/library_core.c -o backend/library_core
    if [ $? -ne 0 ]; then
        echo "[!] GCC compilation failed."
        exit 1
    fi
    echo "[+] C backend compiled successfully."
fi

# 2. Launch web server using Node.js
if command -v node &>/dev/null; then
    echo "[*] Starting web server using Node.js..."
    echo "[*] Access the interface at: http://localhost:8080"
    node server.js
else
    echo "[!] Node.js was not found in PATH. Please install Node.js."
    exit 1
fi
