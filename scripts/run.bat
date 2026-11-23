@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "CONFIG=Release"
if /I "%~1"=="Debug" set "CONFIG=Debug"
if not "%~1"=="" if /I not "%~1"=="Debug" if /I not "%~1"=="Release" exit /b 2
call "%~dp0build.bat" "%CONFIG%" || exit /b 1
pushd "%~dp0.." || exit /b 1
"build\local\%CONFIG%\SnowParkour.exe"
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
