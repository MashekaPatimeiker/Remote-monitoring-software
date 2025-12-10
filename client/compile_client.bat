@echo off
echo =============================================
echo    Client Monitor - Modular Build
echo =============================================
echo.

echo Compiling source files...
echo.

gcc -c main.c -o main.o -I. -O2 -w
gcc -c window_manager.c -o window_manager.o -I. -O2 -w
gcc -c ui_manager.c -o ui_manager.o -I. -O2 -w
gcc -c client.c -o client.o -I. -O2 -w
gcc -c scanner.c -o scanner.o -I. -O2 -w
gcc -c network_utils.c -o network_utils.o -I. -O2 -w

if errorlevel 1 (
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

echo Linking...
gcc main.o window_manager.o ui_manager.o client.o scanner.o network_utils.o ^
    -o ProcessMonitorClient.exe ^
    -lws2_32 -lcomctl32 -liphlpapi -lgdi32 -luser32 -lkernel32 -mwindows -s

if errorlevel 1 (
    echo ERROR: Linking failed!
    pause
    exit /b 1
)

del *.o 2>nul

echo.
echo =============================================
echo    BUILD SUCCESSFUL!
echo =============================================
echo.
echo Run: ProcessMonitorClient.exe
echo.
pause