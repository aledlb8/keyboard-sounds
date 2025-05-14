/**
 * @file main.cpp
 * @brief Entry point for the keyboard sounds application
 */
#include "Application.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <stdexcept>

/**
 * @brief Windows entry point
 */
int WINAPI WinMain(HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */,
                   LPSTR /* lpCmdLine */, int /* nCmdShow */)
{
    // Redirect stderr to a file for debugging
    std::ofstream logFile("keyboard_sounds_debug.log");
    if (logFile.is_open()) {
        std::cerr.rdbuf(logFile.rdbuf());
        std::cerr << "Keyboard Sounds application starting..." << std::endl;
    }

    try
    {
        Application app("sounds");
        return app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        MessageBoxA(NULL, e.what(), "Error", MB_ICONERROR);
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught" << std::endl;
        MessageBoxA(NULL, "An unknown error occurred", "Error", MB_ICONERROR);
        return 1;
    }
}