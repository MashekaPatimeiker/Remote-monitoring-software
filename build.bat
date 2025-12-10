@echo off
gcc -o bin/server_gui.exe server_gui.c ^
    -lws2_32 ^
    -lcomctl32 ^
    -lpsapi ^
    -liphlpapi ^
    -mwindows ^
    -Wl,--subsystem,windows

if errorlevel 1 (
    pause
    exit /b 1
)

echo.
gcc -o bin/client_gui.exe client_gui.c -lws2_32 -lcomctl32 -liphlpapi -mwindows -Wl,--subsystem,windows

if errorlevel 1 (
    pause
    exit /b 1
)
