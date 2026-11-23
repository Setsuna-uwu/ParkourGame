@echo off
rem Add the non-redistributable Visual Studio debug CRT to PATH for local runs.
rem This file is only used by Debug builds; Release never needs it.
findstr /C:"CMAKE_GENERATOR:INTERNAL=Ninja" "%~dp0..\build\CMakeCache.txt" >nul && exit /b 0
if defined SP_DEBUG_ENV_READY exit /b 0

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1

set "VSPATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH exit /b 1

set "DEBUG_CRT="
for /d %%D in ("%VSPATH%\VC\Redist\MSVC\*") do (
    if exist "%%D\debug_nonredist\x64" (
        for /d %%C in ("%%D\debug_nonredist\x64\Microsoft.VC*.DebugCRT") do set "DEBUG_CRT=%%C"
    )
)

set "DEBUG_UCRT="
for /d %%K in ("%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.*") do (
    if exist "%%K\x64\ucrt\ucrtbased.dll" set "DEBUG_UCRT=%%K\x64\ucrt"
)

if not defined DEBUG_CRT (
    echo [debug-env] Visual Studio DebugCRT directory not found.
    exit /b 1
)
if not defined DEBUG_UCRT (
    echo [debug-env] Windows SDK ucrtbased.dll directory not found.
    exit /b 1
)

set "PATH=%DEBUG_CRT%;%DEBUG_UCRT%;%PATH%"
set "SP_DEBUG_ENV_READY=1"
echo [debug-env] debug CRT path prepared
exit /b 0
