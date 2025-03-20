#include "Application.h"
#include <windows.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <commctrl.h>
#include <limits>
#include <uxtheme.h>

// Global volume variable (0–100)
int g_volume = 50;

Application::Application(const std::string &soundFolder)
    : soundManager(soundFolder), hookManager(soundManager, soundPlayer)
{
}

// Forward declaration of the window procedure.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int Application::run()
{
    // Seed random number generator.
    srand(static_cast<unsigned>(time(nullptr)));

    std::string defaultFolder = "sounds";
    std::vector<std::string> soundPacks;
    for (const auto &entry : std::filesystem::directory_iterator(defaultFolder))
    {
        if (entry.is_directory())
        {
            soundPacks.push_back(entry.path().string());
        }
    }
    if (soundPacks.empty())
    {
        MessageBox(NULL, "No sound packs found in 'sounds' folder.", "Error", MB_ICONERROR);
        return 1;
    }
    // By default, use the first pack.
    soundManager.setFolderPath(soundPacks[0]);
    if (!soundManager.loadSounds())
    {
        MessageBox(NULL, ("Failed to load default sound pack: " + soundPacks[0]).c_str(), "Error", MB_ICONERROR);
        return 1;
    }

    // Initialize common controls for trackbar.
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Register window class.
    const char *CLASS_NAME = "SoundAppWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create the main window.
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Keyboard Sounds",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 300,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        this // Pass pointer to this Application instance.
    );
    if (hwnd == NULL)
    {
        MessageBox(NULL, "Failed to create window.", "Error", MB_ICONERROR);
        return 1;
    }
    ShowWindow(hwnd, SW_SHOW);

    // Install the keyboard hook.
    if (!hookManager.installHook())
    {
        MessageBox(NULL, "Error installing keyboard hook.", "Error", MB_ICONERROR);
        return 1;
    }

    // Message loop.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    hookManager.uninstallHook();
    return 0;
}

void Application::updateSoundPack(const std::string &pack)
{
    soundManager.setFolderPath(pack);
    if (soundManager.loadSounds())
    {
        // Optionally provide feedback via MessageBox or UI.
    }
    else
    {
        MessageBox(NULL, ("Failed to load sound pack from: " + pack).c_str(), "Error", MB_ICONERROR);
    }
}

// Window procedure to handle our simple UI.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Application *app = reinterpret_cast<Application *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Retrieve and store Application pointer.
        LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        app = reinterpret_cast<Application *>(pcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)app);

        HWND hStatic = CreateWindow(
            "STATIC", "Keyboard Sounds",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            20, 5, 350, 30,
            hwnd,
            (HMENU)3,
            GetModuleHandle(NULL),
            NULL);
        HFONT hFont = CreateFont(
            24, 0, 0, 0,
            FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            VARIABLE_PITCH, TEXT("Segoe UI"));
        SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hCombo = CreateWindow(
            "COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            20, 50, 350, 200,
            hwnd,
            (HMENU)1, // Control ID 1
            GetModuleHandle(NULL),
            NULL);
        SetWindowTheme(hCombo, L"Explorer", NULL);

        std::string defaultFolder = "sounds";
        for (const auto &entry : std::filesystem::directory_iterator(defaultFolder))
        {
            if (entry.is_directory())
            {
                std::string pack = entry.path().string();
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)pack.c_str());
            }
        }
        SendMessage(hCombo, CB_SETCURSEL, 0, 0);

        // Create a trackbar for volume control.
        HWND hTrackbar = CreateWindowEx(
            0,
            "msctls_trackbar32",
            NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            20, 100, 350, 30,
            hwnd,
            (HMENU)2, // Control ID 2
            GetModuleHandle(NULL),
            NULL);
        // Apply Explorer theme to the trackbar.
        SetWindowTheme(hTrackbar, L"Explorer", NULL);
        // Set trackbar range (0-100) and initial position (50).
        SendMessage(hTrackbar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessage(hTrackbar, TBM_SETPOS, TRUE, 50);

        return 0;
    }
    case WM_HSCROLL:
    {
        // Handle trackbar (volume slider) changes.
        HWND hTrackbar = (HWND)lParam;
        int id = GetDlgCtrlID(hTrackbar);
        if (id == 2)
        {
            int pos = (int)SendMessage(hTrackbar, TBM_GETPOS, 0, 0);
            g_volume = pos;
            // Optionally update window title with volume.
            char title[256];
            sprintf(title, "Sound Pack Selector - Volume: %d", g_volume);
            SetWindowText(hwnd, title);
        }
        break;
    }
    case WM_COMMAND:
    {
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            // The combobox selection has changed.
            HWND hCombo = (HWND)lParam;
            int index = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            char buffer[256] = {0};
            SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
            std::string selectedPack(buffer);
            if (app)
            {
                app->updateSoundPack(selectedPack);
            }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}