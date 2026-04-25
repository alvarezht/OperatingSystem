@echo off
setlocal

if not exist build (
    mkdir build
)

set "CC="
set "MODE="

where gcc >nul 2>&1
if %errorlevel%==0 (
    set "CC=gcc"
    set "MODE=gnu"
)

if not defined CC (
    where clang >nul 2>&1
    if %errorlevel%==0 (
        set "CC=clang"
        set "MODE=gnu"
    )
)

if not defined CC (
    where cl >nul 2>&1
    if %errorlevel%==0 (
        set "CC=cl"
        set "MODE=msvc"
    )
)

if not defined CC (
    echo Error: No se encontro compilador C en PATH.
    echo Instala MinGW ^(gcc^), LLVM ^(clang^) o Visual Studio Build Tools ^(cl^).
    exit /b 1
)

echo [1/2] Compilando proyecto...
if /I "%MODE%"=="gnu" (
    %CC% -Wall -Wextra -std=c11 -Iinclude main.c src\process.c src\dataset.c src\scheduler.c src\report.c -o build\lab008_scheduling.exe
    if errorlevel 1 (
        echo Error: Fallo la compilacion con %CC%.
        exit /b 1
    )
) else (
    %CC% /nologo /W4 /I include main.c src\process.c src\dataset.c src\scheduler.c src\report.c /Fe:build\lab008_scheduling.exe
    if errorlevel 1 (
        echo Error: Fallo la compilacion con cl.
        exit /b 1
    )
)

echo [2/2] Ejecutando simulacion...
build\lab008_scheduling.exe

endlocal
