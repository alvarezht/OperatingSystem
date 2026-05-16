@echo off
gcc -Wall -Wextra -std=c11 -o vat_sim.exe main.c
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)
echo Build succeeded: vat_sim.exe
