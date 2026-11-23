@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
pushd "%ROOT%" >nul || exit /b 1

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"
if /I "%CONFIG%"=="debug" set "CONFIG=Debug"
if /I "%CONFIG%"=="release" set "CONFIG=Release"
if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="Release" (
    echo [build] usage: scripts\build.bat [Debug^|Release]
    popd
    exit /b 2
)

if not exist "third_party\easyx\include\easyx.h" (
    echo [build] EasyX SDK missing under third_party\easyx
    popd
    exit /b 1
)

call :find_vs
if errorlevel 1 (
    where g++ >nul 2>&1 || exit /b 1
    where ninja >nul 2>&1 || exit /b 1
    cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ || exit /b 1
    goto compile
)

set "GENERATOR="
if "%VSMAJOR%"=="18" set "GENERATOR=Visual Studio 18 2026"
if "%VSMAJOR%"=="17" set "GENERATOR=Visual Studio 17 2022"
if "%VSMAJOR%"=="16" set "GENERATOR=Visual Studio 16 2019"
if not defined GENERATOR (
    echo [build] unsupported Visual Studio major version: %VSMAJOR%
    popd
    exit /b 1
)

echo [build] config=%CONFIG%  generator=%GENERATOR%


cmake -S . -B build -G "%GENERATOR%" -A x64 || goto :fail
:compile
cmake --build build --config %CONFIG% --parallel || goto :fail

echo [build] OK -^> build\%CONFIG%\SnowParkour.exe
popd
exit /b 0

:find_vs
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
set "VSPATH="
set "VSMAJOR="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
for /f "usebackq tokens=1 delims=." %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do set "VSMAJOR=%%i"
if not defined VSPATH exit /b 1
if not defined VSMAJOR exit /b 1
exit /b 0

:fail
echo [build] FAILED
popd
exit /b 1
