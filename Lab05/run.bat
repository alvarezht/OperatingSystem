@echo off
setlocal

set THREADS=%1
if "%THREADS%"=="" set THREADS=4

set LOG_FILE=%2
if "%LOG_FILE%"=="" set LOG_FILE=access.log

where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: gcc not found. Install MinGW gcc and add it to PATH.
    goto :end
)

echo Compiling with gcc...
gcc -std=c11 -O2 -Wall -Wextra -pedantic main_windows.c log_processor.c -o log_analyzer_windows.exe
if errorlevel 1 goto :build_error
log_analyzer_windows.exe %LOG_FILE% %THREADS%
goto :end

:build_error
echo Build failed.

:end
endlocal
