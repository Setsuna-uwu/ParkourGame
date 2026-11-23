cmake_minimum_required(VERSION 3.21)

project(SnowParkour VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "SnowParkour targets Windows 10/11 only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "EasyX is vendored for x64 only. Configure with -A x64 or an x64 toolchain.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SP_BUILD_TESTS "Build headless core unit tests" ON)
option(SP_ENABLE_DEBUG_HUD "Enable the F1 debug HUD and collision boxes" ON)

set(SP_EASYX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/easyx" CACHE PATH "Vendored EasyX SDK root")

add_library(sp_core STATIC src/game.c src/terrain.c)
target_include_directories(sp_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(sp_core PUBLIC c_std_11)

if(MSVC)
    target_compile_options(sp_core PRIVATE /W4 /utf-8)
    target_compile_definitions(sp_core PUBLIC _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(sp_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(SnowParkour src/main.c src/windows.cpp src/draw.cpp)
target_link_libraries(SnowParkour PRIVATE sp_core)
target_include_directories(SnowParkour PRIVATE
    "${SP_EASYX_ROOT}/include"
)

if(MSVC)
    target_compile_options(SnowParkour PRIVATE /W4 /utf-8 /wd4201)
    target_compile_definitions(SnowParkour PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS WINVER=0x0A00 _WIN32_WINNT=0x0A00)
    target_link_directories(SnowParkour PRIVATE "${SP_EASYX_ROOT}/lib/x64")
    target_link_libraries(SnowParkour PRIVATE
        "${SP_EASYX_ROOT}/lib/x64/EasyXw.lib"
        winmm user32 gdi32 gdiplus msimg32
    )
    target_link_options(SnowParkour PRIVATE /ENTRY:mainCRTStartup)
    set_target_properties(SnowParkour PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    target_compile_options(SnowParkour PRIVATE -Wall -Wextra)
    target_compile_definitions(SnowParkour PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS WINVER=0x0A00 _WIN32_WINNT=0x0A00)
    target_include_directories(SnowParkour PRIVATE "${SP_EASYX_ROOT}/include")
    target_link_directories(SnowParkour PRIVATE "${SP_EASYX_ROOT}/lib/mingw/x64")
    target_link_libraries(SnowParkour PRIVATE
        "${SP_EASYX_ROOT}/lib/mingw/x64/libeasyxw.a"
        winmm user32 gdi32 gdiplus msimg32
    )
    target_link_options(SnowParkour PRIVATE -mwindows)
endif()

if(SP_ENABLE_DEBUG_HUD)
    target_compile_definitions(sp_core PUBLIC SP_DEBUG_HUD=1)
    target_compile_definitions(SnowParkour PRIVATE SP_DEBUG_HUD=1)
else()
    target_compile_definitions(sp_core PUBLIC SP_DEBUG_HUD=0)
    target_compile_definitions(SnowParkour PRIVATE SP_DEBUG_HUD=0)
endif()

if(SP_BUILD_TESTS)
    enable_testing()

    add_executable(sp_tests EXCLUDE_FROM_ALL tests/test.c)
    target_link_libraries(sp_tests PRIVATE sp_core)
    if(MSVC)
        target_compile_options(sp_tests PRIVATE /W4 /utf-8)
        target_compile_definitions(sp_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(sp_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_link_libraries(sp_tests PRIVATE m)
    endif()

    add_test(NAME core_all COMMAND sp_tests)
    add_test(NAME core_physics COMMAND sp_tests physics)
    add_test(NAME core_collision COMMAND sp_tests collision)
    add_test(NAME core_terrain COMMAND sp_tests terrain)
    add_test(NAME core_difficulty COMMAND sp_tests difficulty)
    add_test(NAME core_score COMMAND sp_tests score)
    add_test(NAME core_game COMMAND sp_tests game)
endif()

message(STATUS "SnowParkour ${PROJECT_VERSION}")
message(STATUS "  EasyX root : ${SP_EASYX_ROOT}")
message(STATUS "  Debug HUD  : ${SP_ENABLE_DEBUG_HUD}")
message(STATUS "  Tests      : ${SP_BUILD_TESTS}")
