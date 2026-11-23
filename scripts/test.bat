@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "SCRIPT_DIR=%~dp0"
set "CONFIG=Release"
set "FILTER="

:parse
if "%~1"=="" goto ready
if /I "%~1"=="--filter" (
    if "%~2"=="" goto usage
    set "FILTER=%~2"
    shift
    shift
    goto parse
)
if /I "%~1"=="Debug" (
    set "CONFIG=Debug"
    shift
    goto parse
)
if /I "%~1"=="Release" (
    set "CONFIG=Release"
    shift
    goto parse
)
:usage
echo [test] usage: scripts\test.bat [Debug^|Release] [--filter suite]
exit /b 2

:ready
call "%SCRIPT_DIR%build.bat" "%CONFIG%" tests || exit /b 1
set "TEST_EXE=%SCRIPT_DIR%..\build\local\%CONFIG%\sp_tests.exe"
if not exist "%TEST_EXE%" exit /b 1
if defined FILTER (
    "%TEST_EXE%" "%FILTER%"
) else (
    "%TEST_EXE%"
)
exit /b
