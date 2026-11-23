@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "CONFIG=Release"
set "FILTER="

:parse
if "%~1"=="" goto :ready
if /I "%~1"=="--filter" (
    set "FILTER=%~2"
    shift
    shift
    goto :parse
)
if /I "%~1"=="Debug" (
    set "CONFIG=Debug"
    shift
    goto :parse
)
if /I "%~1"=="Release" (
    set "CONFIG=Release"
    shift
    goto :parse
)
echo [test] unknown argument: %~1
echo [test] usage: scripts\test.bat [Debug^|Release] [--filter suite]
exit /b 2

:ready
call "%SCRIPT_DIR%build.bat" %CONFIG% || exit /b 1

cmake --build "%SCRIPT_DIR%..\build" --config %CONFIG% --target sp_tests || exit /b 1

if /I "%CONFIG%"=="Debug" (
    call "%SCRIPT_DIR%_debug_env.bat" || exit /b 1
)

set "TEST_EXE=%SCRIPT_DIR%..\build\%CONFIG%\sp_tests.exe"
if not exist "%TEST_EXE%" (
    echo [test] missing %TEST_EXE%
    exit /b 1
)

echo [test] running %CONFIG%\sp_tests.exe %FILTER%
if defined FILTER (
    "%TEST_EXE%" %FILTER%
) else (
    "%TEST_EXE%"
)
if errorlevel 1 (
    echo [test] FAILED
    exit /b 1
)
echo [test] OK
exit /b 0
