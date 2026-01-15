# SPDX-FileCopyrightText: 2026 Juan Medina
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13)

project(pxe
        VERSION 0.1.0.0
        DESCRIPTION "A pixel oriented game engine"
        LANGUAGES CXX
)

# Auto-detect platform for Emscripten builds
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    if (NOT DEFINED PLATFORM)
        set(PLATFORM "Web" CACHE STRING "Target platform")
    endif ()
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE APP_PUBLIC_HEADERS "include/pxe/*.hpp")
file(GLOB_RECURSE APP_SOURCE_FILES "src/pxe/*.cpp")

set(EMSCRIPTEN FALSE)
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(EMSCRIPTEN TRUE)
endif ()

add_library(${PROJECT_NAME} STATIC ${APP_SOURCE_FILES})

if (APP_PUBLIC_HEADERS)
    target_sources(${PROJECT_NAME} PUBLIC ${APP_PUBLIC_HEADERS})
endif ()

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            NOUSER
            NOGDI
    )
endif ()

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif ()

# spdlog
add_subdirectory(external/spdlog)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog_header_only)
target_compile_definitions(${PROJECT_NAME} PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
target_compile_definitions(${PROJECT_NAME} PUBLIC SPDLOG_USE_STD_FORMAT)

# raylib
add_subdirectory(external/raylib)
target_link_libraries(${PROJECT_NAME} PUBLIC raylib)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/raylib/src)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/raygui/src)

# jsoncons
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/jsoncons/include)

# boxer - not on emscripten
if (NOT EMSCRIPTEN)
    add_subdirectory(external/boxer)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boxer)
endif ()

