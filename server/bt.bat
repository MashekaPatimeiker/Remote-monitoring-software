@echo off
chcp 1251 > nul
title Process Monitor Server Compiler

echo =============================================
echo    Process Monitor Server Compilation
echo    Using MinGW (gcc)
echo =============================================
echo.

echo Step 1: Checking source files...
echo.

:: Check all required source files
if not exist "server.c" (
    echo ERROR: server.c not found!
    pause
    exit /b 1
)

if not exist "window_manager.c" (
    echo ERROR: window_manager.c not found!
    pause
    exit /b 1
)

if not exist "ui_manager.c" (
    echo ERROR: ui_manager.c not found!
    pause
    exit /b 1
)

if not exist "server_manager.c" (
    echo ERROR: server_manager.c not found!
    pause
    exit /b 1
)

if not exist "process_manager.c" (
    echo ERROR: process_manager.c not found!
    pause
    exit /b 1
)

echo Step 2: Compiling source files...
echo.

:: Compile all .c files
gcc -c server.c -o server.o -I. -O2 -Wall
gcc -c window_manager.c -o window_manager.o -I. -O2 -Wall
gcc -c ui_manager.c -o ui_manager.o -I. -O2 -Wall
gcc -c server_manager.c -o server_manager.o -I. -O2 -Wall
gcc -c process_manager.c -o process_manager.o -I. -O2 -Wall

if errorlevel 1 (
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

echo Step 3: Linking executable...
echo.

:: Link with Windows libraries
gcc server.o window_manager.o ui_manager.o server_manager.o process_manager.o ^
    -o ProcessMonitorServer.exe ^
    -lws2_32 -lcomctl32 -lpsapi -lgdi32 -mwindows -s

if errorlevel 1 (
    echo ERROR: Linking failed!
    pause
    exit /b 1
)

echo Step 4: Cleaning up...
echo.

:: Remove object files
del *.o 2>nul

echo =============================================
echo    SUCCESS: Compilation completed!
echo    Output: ProcessMonitorServer.exe
echo =============================================
echo.
echo To run the program:
echo   ProcessMonitorServer.exe
echo.
echo Or double-click ProcessMonitorServer.exe
echo.
pause