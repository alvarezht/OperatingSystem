@echo off
setlocal

echo [INFO] Compilando Parking Lot UI para Windows...
gcc main.c parking_lot.c -o parking_gui.exe -I. -L. -lraylib -lopengl32 -lgdi32 -lwinmm -pthread

if errorlevel 1 (
    echo [ERROR] Fallo la compilacion.
    exit /b 1
)

echo [INFO] Ejecutando parking_gui.exe...
parking_gui.exe

if errorlevel 1 (
    echo [ERROR] La ejecucion termino con error.
    exit /b 1
)

echo [INFO] Finalizado correctamente.
exit /b 0
