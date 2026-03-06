@echo off
setlocal

set "BUILD_DIR=build"
set "BUILD_TYPE=Release"

if not "%~1"=="" set "BUILD_TYPE=%~1"

echo Configuring with CMake + Ninja (%BUILD_TYPE%)...
cmake -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 goto :error

echo Building...
cmake --build "%BUILD_DIR%"
if errorlevel 1 goto :error

echo.
echo Build completed successfully.
echo Executable: %CD%\%BUILD_DIR%\keyboard-sounds.exe
echo.
exit /b 0

:error
echo.
echo Build failed.
exit /b 1
