@echo off
echo Building...

:: Check if build directory exists, create if not
if not exist build mkdir build

:: Go to build directory and configure with CMake
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

:: Build the project
cmake --build . --config Release

:: Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build completed successfully!
    echo The executable is located in: %CD%\keyboard-sounds.exe
    echo.
    echo You may need to copy SFML DLL files to this directory to run the application.
) else (
    echo.
    echo Build failed with error code: %ERRORLEVEL%
    echo Please check the error messages above.
)

:: Return to original directory
cd ..

echo Done.
pause