@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "ROOT=%~dp0.."
set "CONFIG=Release"
if /I "%~1"=="Debug" set "CONFIG=Debug"
if not "%~1"=="" if /I not "%~1"=="Release" if /I not "%~1"=="Debug" goto usage
set "TARGET=%~2"
if not defined TARGET set "TARGET=game"
if /I not "%TARGET%"=="game" if /I not "%TARGET%"=="tests" goto usage

set "COMPILER="
for /f "delims=" %%I in ('gcc -dumpmachine 2^>nul') do if "%%I"=="x86_64-w64-mingw32" set "COMPILER=mingw"
for /f "delims=" %%I in ('g++ -dumpmachine 2^>nul') do set "CXX_TARGET=%%I"
if not "%CXX_TARGET%"=="x86_64-w64-mingw32" set "COMPILER="
if not defined COMPILER (
    call :setup_msvc
    if errorlevel 1 (
        echo [build] No x64 compiler found. Add MinGW-w64 gcc and g++ to PATH,
        echo [build] or install Visual Studio Build Tools with Desktop development with C++.
        exit /b 1
    )
    set "COMPILER=msvc"
)

set "OUT=%ROOT%\build\local\%CONFIG%"
if not exist "%OUT%\" mkdir "%OUT%" || exit /b 1
pushd "%OUT%" || exit /b 1
set "CORE=%ROOT%\src"
set "EASYX=%ROOT%\third_party\easyx"
echo [build] %COMPILER% %CONFIG% %TARGET%
if "%COMPILER%"=="mingw" (call :mingw) else (call :msvc)
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%

:mingw
set "OPT=-O2 -DNDEBUG"
if "%CONFIG%"=="Debug" set "OPT=-O0 -g"
set "CFLAGS=-std=c11 -Wall -Wextra %OPT%"
if /I "%TARGET%"=="tests" (
    gcc %CFLAGS% -I"%CORE%" "%CORE%\game.c" "%CORE%\terrain.c" "%ROOT%\tests\test.c" -lm -static -o sp_tests.exe
    exit /b
)
gcc %CFLAGS% -c "%CORE%\game.c" "%CORE%\terrain.c" "%CORE%\main.c" || exit /b 1
g++ -std=c++17 -Wall -Wextra -Wno-unknown-pragmas %OPT% -DUNICODE -D_UNICODE -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -I"%EASYX%\include" "%CORE%\windows.cpp" "%CORE%\draw.cpp" main.o game.o terrain.o "%EASYX%\lib\mingw\x64\libeasyxw.a" -lwinmm -luser32 -lgdi32 -lgdiplus -lmsimg32 -mwindows -static -o SnowParkour.exe
exit /b

:msvc
set "OPT=/O2 /MT /DNDEBUG"
if "%CONFIG%"=="Debug" set "OPT=/Od /Zi /MTd"
set "FLAGS=/nologo /W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS %OPT%"
if /I "%TARGET%"=="tests" (
    cl %FLAGS% /std:c11 /I"%CORE%" "%CORE%\game.c" "%CORE%\terrain.c" "%ROOT%\tests\test.c" /Fe:sp_tests.exe
    exit /b
)
cl %FLAGS% /std:c11 /c "%CORE%\game.c" "%CORE%\terrain.c" "%CORE%\main.c" || exit /b 1
cl %FLAGS% /std:c++17 /wd4201 /DUNICODE /D_UNICODE /DWINVER=0x0A00 /D_WIN32_WINNT=0x0A00 /I"%EASYX%\include" "%CORE%\windows.cpp" "%CORE%\draw.cpp" main.obj game.obj terrain.obj /Fe:SnowParkour.exe /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /LIBPATH:"%EASYX%\lib\x64" EasyXw.lib winmm.lib user32.lib gdi32.lib gdiplus.lib msimg32.lib
exit /b

:setup_msvc
where cl >nul 2>&1
if not errorlevel 1 if /I "%VSCMD_ARG_TGT_ARCH%"=="x64" exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
set "VSPATH="
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%I"
if not defined VSPATH exit /b 1
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
exit /b

:usage
echo [build] usage: scripts\build.bat [Debug^|Release] [game^|tests]
exit /b 2
