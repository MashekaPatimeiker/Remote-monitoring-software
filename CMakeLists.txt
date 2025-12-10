cmake_minimum_required(VERSION 3.20)
project(RemoteMonitor)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Настройки для Windows
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600)
    set(CMAKE_RC_COMPILER windres.exe)
endif()

# Общие файлы
set(COMMON_SOURCES
        common/protocol.h
        common/process_info.h
        common/process_info.c
)

target_include_directories(agent PRIVATE common client)
target_link_libraries(agent
        ws2_32
        user32
        gdi32
        comctl32
        psapi
        userenv
        wtsapi32
)


target_include_directories(server PRIVATE common server)
target_link_libraries(server
        ws2_32
        user32
        gdi32
        comctl32
        psapi
        userenv
        wtsapi32
)