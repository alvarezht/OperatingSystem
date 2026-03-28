@echo off
gcc main.c bridge.c -pthread -o bridge.exe
if errorlevel 1 (
    echo Error de compilacion.
    exit /b 1
)
bridge.exe
