@echo off
setlocal
cd /d "%~dp0"
set "ROOT=%CD%"
set "SDK=%ROOT%\third_party\easyx"
for /f "delims=" %%I in ('gcc -dumpmachine 2^>nul') do if "%%I"=="x86_64-w64-mingw32" goto mingw

if /i "%VSCMD_ARG_TGT_ARCH%"=="x64" goto msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto missing
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT goto missing
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || goto fail

:msvc
rem Keep MSVC object files outside the checkout. Each checkout has its own cache.
set "CHECKOUT=%ROOT::=%"
set "OBJDIR=%TEMP%\SnowParkour\%CHECKOUT%"
if not exist "%OBJDIR%" mkdir "%OBJDIR%" || goto fail
pushd "%OBJDIR%" || goto fail
cl /nologo /utf-8 /W4 /wd4201 /std:c11 /EHsc /DUNICODE /D_UNICODE /I"%SDK%\include" "%ROOT%\src\*.c" "%ROOT%\src\draw.cpp" /Fe:"%ROOT%\SnowParkour.exe" /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /LIBPATH:"%SDK%\lib\x64" winmm.lib user32.lib gdi32.lib gdiplus.lib msimg32.lib || goto fail
popd
goto run

:mingw
gcc -Wall -Wextra -Wno-unknown-pragmas -DUNICODE -D_UNICODE -I"%SDK%\include" src\*.c src\draw.cpp "%SDK%\lib\mingw\x64\libeasyxw.a" -lstdc++ -lwinmm -luser32 -lgdi32 -lgdiplus -lmsimg32 -static -mwindows -o SnowParkour.exe || goto fail

:run
cd /d "%ROOT%" || goto fail
"%ROOT%\SnowParkour.exe"
exit /b %errorlevel%

:missing
echo Install x64 MinGW-w64 or Visual Studio with Desktop development with C++.
:fail
echo Build failed. See the message above.
pause
exit /b 1
