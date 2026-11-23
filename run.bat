@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build || goto fail
pushd build || goto fail
set "SDK=..\third_party\easyx"
for /f "delims=" %%I in ('gcc -dumpmachine 2^>nul') do if "%%I"=="x86_64-w64-mingw32" goto mingw

if /i "%VSCMD_ARG_TGT_ARCH%"=="x64" goto msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto missing
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT goto missing
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || goto fail

:msvc
cl /nologo /utf-8 /W4 /std:c11 /c ..\src\main.c ..\src\game.c ..\src\terrain.c || goto fail
cl /nologo /utf-8 /W4 /wd4201 /EHsc /DUNICODE /D_UNICODE /I"%SDK%\include" ..\src\draw.cpp main.obj game.obj terrain.obj /Fe:SnowParkour.exe /link /LIBPATH:"%SDK%\lib\x64" winmm.lib user32.lib gdi32.lib gdiplus.lib msimg32.lib || goto fail
goto run

:mingw
gcc -std=c11 -Wall -Wextra -c ..\src\main.c ..\src\game.c ..\src\terrain.c || goto fail
g++ -std=c++17 -Wall -Wextra -Wno-unknown-pragmas -DUNICODE -D_UNICODE -I"%SDK%\include" ..\src\draw.cpp main.o game.o terrain.o "%SDK%\lib\mingw\x64\libeasyxw.a" -lwinmm -luser32 -lgdi32 -lgdiplus -lmsimg32 -static -o SnowParkour.exe || goto fail

:run
SnowParkour.exe
exit /b %errorlevel%

:missing
echo Install x64 MinGW-w64 or Visual Studio with Desktop development with C++.
:fail
echo Build failed. See the message above.
pause
exit /b 1
