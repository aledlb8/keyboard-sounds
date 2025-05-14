/**
 * @file Application.cpp
 * @brief Implementation of the Application class
 */
#include "Application.h"
#include "Utils.h"
#include <windows.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <commctrl.h>
#include <limits>
#include <uxtheme.h>
#include <stdexcept>

// Global volume variable (0–100)
int g_volume = 50;

// Modern UI colors
const COLORREF APP_BG_COLOR = RGB(245, 245, 250);
const COLORREF APP_TEXT_COLOR = RGB(40, 45, 60);
const COLORREF APP_ACCENT_COLOR = RGB(65, 105, 225);  // Royal Blue
const COLORREF APP_BORDER_COLOR = RGB(220, 220, 230);
const COLORREF APP_HIGHLIGHT_COLOR = RGB(230, 240, 255);

// UI metrics
const int WINDOW_WIDTH = 550;
const int WINDOW_HEIGHT = 350;
const int MARGIN = 20;
const int CONTROL_HEIGHT = 30;
const int LABEL_WIDTH = 130;
const int CONTROL_WIDTH = WINDOW_WIDTH - (2 * MARGIN) - LABEL_WIDTH - 20;
const int SPACING = 20;

Application::Application(const std::string &soundFolder)
    : soundFolder_(soundFolder),
      soundManager_(std::make_unique<SoundManager>(soundFolder)),
      soundPlayer_(std::make_unique<SFMLSoundPlayer>()),
      hwnd_(nullptr),
      comboBox_(nullptr),
      volumeSlider_(nullptr),
      optimizationCombo_(nullptr),
      volume_(DEFAULT_VOLUME),
      latencyOptimizationLevel_(DEFAULT_OPTIMIZATION)
{
    // Initialize common controls for trackbar and modern UI elements
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Create the hook manager after we have the sound manager and player
    hookManager_ = std::make_unique<KeyboardHookManager>(*soundManager_, *soundPlayer_);

    // Set initial volume
    soundPlayer_->setVolume(volume_);
    
    // Set initial optimization level
    hookManager_->setLatencyOptimization(latencyOptimizationLevel_);
}

Application::~Application()
{
    // Ensure hook is uninstalled when application is destroyed
    if (hookManager_)
    {
        hookManager_->uninstallHook();
    }
    
    // Clean up fonts and other resources
    for (HFONT font : fonts_) {
        if (font) DeleteObject(font);
    }
}

int Application::run()
{
    // Seed random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    // Load sound packs
    if (!loadSoundPacks())
    {
        MessageBoxW(nullptr, L"No sound packs found in sounds folder.", L"Error", MB_ICONERROR);
        return 1;
    }

    // Initialize the window
    if (!initializeWindow())
    {
        MessageBoxW(nullptr, L"Failed to create application window.", L"Error", MB_ICONERROR);
        return 1;
    }

    // Load the default sound pack
    if (!soundPacks_.empty())
    {
        // Use the first sound pack by default
        soundManager_->setFolderPath(soundPacks_[0]);
        std::cerr << "Setting default sound pack: " << soundPacks_[0] << std::endl;
    }
    
    if (!soundManager_->loadSounds())
    {
        MessageBoxW(nullptr, L"Failed to load default sound pack.", L"Error", MB_ICONERROR);
        return 1;
    }

    // Install keyboard hook
    if (!hookManager_->installHook())
    {
        MessageBoxW(nullptr, L"Error installing keyboard hook.", L"Error", MB_ICONERROR);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

bool Application::loadSoundPacks()
{
    soundPacks_.clear();

    try
    {
        // Validate sounds folder exists
        if (!std::filesystem::exists(soundFolder_))
        {
            std::cerr << "Error: Sounds folder does not exist: " << soundFolder_ << std::endl;
            return false;
        }

        // Find all subdirectories in the sounds folder
        for (const auto &entry : std::filesystem::directory_iterator(soundFolder_))
        {
            if (entry.is_directory())
            {
                soundPacks_.push_back(entry.path().string());
                std::cerr << "Found sound pack: " << entry.path().string() << std::endl;
            }
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Error scanning sound packs directory: " << e.what() << std::endl;
        return false;
    }

    if (soundPacks_.empty())
    {
        std::cerr << "No sound packs found in: " << soundFolder_ << std::endl;
        return false;
    }

    std::cerr << "Loaded " << soundPacks_.size() << " sound packs" << std::endl;
    return true;
}

bool Application::initializeWindow()
{
    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(APP_BG_COLOR);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassW(&wc))
    {
        return false;
    }

    // Get screen dimensions for centering the window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - WINDOW_WIDTH) / 2;
    int windowY = (screenHeight - WINDOW_HEIGHT) / 2;

    // Create the main window
    hwnd_ = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        CLASS_NAME,
        L"Keyboard Sounds",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX, windowY, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        this // Pass pointer to this Application instance
    );

    return hwnd_ != nullptr;
}

bool Application::updateSoundPack(const std::string &pack)
{
    soundManager_->setFolderPath(pack);
    if (soundManager_->loadSounds())
    {
        // Successfully loaded the sounds
        return true;
    }
    else
    {
        std::wstring wMessage = L"Failed to load sound pack from: " + Utils::toWideString(pack);
        MessageBoxW(hwnd_, wMessage.c_str(), L"Error", MB_ICONERROR);
        return false;
    }
}

void Application::setVolume(int volume)
{
    // Clamp volume between 0 and 100
    volume_ = (volume < 0) ? 0 : ((volume > 100) ? 100 : volume);

    // Update the sound player
    if (soundPlayer_)
    {
        soundPlayer_->setVolume(volume_);
    }

    // Update the volume slider if it exists
    if (volumeSlider_)
    {
        SendMessage(volumeSlider_, TBM_SETPOS, TRUE, volume_);
    }
    
    // Update volume label if it exists
    if (volumeValueLabel_)
    {
        wchar_t volumeText[16];
        swprintf_s(volumeText, L"%d%%", volume_);
        SetWindowTextW(volumeValueLabel_, volumeText);
    }
}

int Application::getVolume() const
{
    return volume_;
}

void Application::setLatencyOptimization(int level)
{
    // Clamp level between 0 and 3
    latencyOptimizationLevel_ = (level < 0) ? 0 : ((level > 3) ? 3 : level);
    
    // Update the hook manager
    if (hookManager_)
    {
        hookManager_->setLatencyOptimization(latencyOptimizationLevel_);
    }
    
    // Update the combo box if it exists
    if (optimizationCombo_)
    {
        SendMessage(optimizationCombo_, CB_SETCURSEL, latencyOptimizationLevel_, 0);
    }
}

HFONT Application::createFont(int size, bool bold, bool italic)
{
    HFONT font = CreateFont(
        size, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        italic ? TRUE : FALSE,
        FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    
    if (font) {
        fonts_.push_back(font);
    }
    
    return font;
}

void Application::drawRoundedRect(HDC hdc, RECT rect, COLORREF color, int radius)
{
    HBRUSH brush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    
    // Create rounded rectangle
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    
    // Clean up
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Application *app = nullptr;

    // Get the Application instance associated with this window
    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
        app = reinterpret_cast<Application *>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
        app = reinterpret_cast<Application *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    // Process window messages
    switch (uMsg)
    {
    case WM_CREATE:
    {
        if (!app)
            return -1;
            
        // Set current window text with version info
        SetWindowTextW(hwnd, L"Keyboard Sounds v1.0");
        
        // Create fonts
        HFONT titleFont = app->createFont(28, true, false);
        HFONT labelFont = app->createFont(16, true, false);
        HFONT controlFont = app->createFont(15, false, false);
        
        int yPos = MARGIN;
        
        // Create title label with app logo/icon
        HWND hTitle = CreateWindowW(
            L"STATIC", L"⌨️ Keyboard Sounds",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            MARGIN, yPos, WINDOW_WIDTH - (2 * MARGIN), 40,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont), TRUE);
        
        yPos += 50;
        
        // Create separator line
        HWND hSeparator = CreateWindowW(
            L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            MARGIN, yPos, WINDOW_WIDTH - (2 * MARGIN), 1,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
            
        yPos += SPACING;
        
        // Create sections with proper spacing
        
        // 1. Sound Pack Section
        HWND hPackLabel = CreateWindowW(
            L"STATIC", L"Sound Pack:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            MARGIN, yPos + 5, LABEL_WIDTH, CONTROL_HEIGHT,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(hPackLabel, WM_SETFONT, reinterpret_cast<WPARAM>(labelFont), TRUE);

        app->comboBox_ = CreateWindowW(
            L"COMBOBOX", nullptr,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL,
            MARGIN + LABEL_WIDTH + 10, yPos, CONTROL_WIDTH, CONTROL_HEIGHT * 10,
            hwnd,
            reinterpret_cast<HMENU>(1),
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(app->comboBox_, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);

        // Fill the combobox with sound packs
        for (const auto &pack : app->soundPacks_)
        {
            // Extract just the folder name, not the full path
            std::filesystem::path p(pack);
            std::wstring name = Utils::toWideString(p.filename().string());
            SendMessageW(app->comboBox_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
        }

        // Select the first item
        SendMessage(app->comboBox_, CB_SETCURSEL, 0, 0);
        
        yPos += CONTROL_HEIGHT + SPACING;
        
        // 2. Volume Section
        HWND hVolumeLabel = CreateWindowW(
            L"STATIC", L"Volume:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            MARGIN, yPos + 5, LABEL_WIDTH, CONTROL_HEIGHT,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(hVolumeLabel, WM_SETFONT, reinterpret_cast<WPARAM>(labelFont), TRUE);

        app->volumeSlider_ = CreateWindowW(
            TRACKBAR_CLASSW, nullptr,
            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
            MARGIN + LABEL_WIDTH + 10, yPos, CONTROL_WIDTH - 50, CONTROL_HEIGHT,
            hwnd,
            reinterpret_cast<HMENU>(2),
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(app->volumeSlider_, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);

        // Configure volume slider
        SendMessage(app->volumeSlider_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessage(app->volumeSlider_, TBM_SETTICFREQ, 10, 0);
        SendMessage(app->volumeSlider_, TBM_SETPOS, TRUE, app->volume_);
        
        // Create volume value label
        app->volumeValueLabel_ = CreateWindowW(
            L"STATIC", L"50%",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            WINDOW_WIDTH - MARGIN - 40, yPos + 5, 40, CONTROL_HEIGHT,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(app->volumeValueLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);
        
        yPos += CONTROL_HEIGHT + SPACING;
        
        // 3. Latency Optimization Section
        HWND hOptLabel = CreateWindowW(
            L"STATIC", L"Optimization:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            MARGIN, yPos + 5, LABEL_WIDTH, CONTROL_HEIGHT,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(hOptLabel, WM_SETFONT, reinterpret_cast<WPARAM>(labelFont), TRUE);
            
        app->optimizationCombo_ = CreateWindowW(
            L"COMBOBOX", nullptr,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
            MARGIN + LABEL_WIDTH + 10, yPos, CONTROL_WIDTH, CONTROL_HEIGHT * 5,
            hwnd,
            reinterpret_cast<HMENU>(7),
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(app->optimizationCombo_, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);
            
        // Fill the combo box with optimization levels
        SendMessageW(app->optimizationCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minimal (better compatibility)"));
        SendMessageW(app->optimizationCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Low optimization"));
        SendMessageW(app->optimizationCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Medium (default)"));
        SendMessageW(app->optimizationCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Maximum (lowest latency)"));
        
        // Select the default optimization level
        SendMessage(app->optimizationCombo_, CB_SETCURSEL, app->latencyOptimizationLevel_, 0);
        
        yPos += CONTROL_HEIGHT + SPACING * 1.5;
        
        // Create second separator
        HWND hSeparator2 = CreateWindowW(
            L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            MARGIN, yPos, WINDOW_WIDTH - (2 * MARGIN), 1,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
            
        yPos += SPACING;
        
        // Status label
        HWND hStatusLabel = CreateWindowW(
            L"STATIC", L"Status: Active",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            MARGIN, yPos, WINDOW_WIDTH - (2 * MARGIN), CONTROL_HEIGHT,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
        SendMessage(hStatusLabel, WM_SETFONT, reinterpret_cast<WPARAM>(labelFont), TRUE);
        
        // Set custom colors for controls (will apply on first paint)
        app->SetControlColors(hwnd);

        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        // Set custom colors for static controls
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, APP_TEXT_COLOR);
        SetBkColor(hdcStatic, APP_BG_COLOR);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    {
        // Set custom colors for other controls
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, APP_TEXT_COLOR);
        SetBkColor(hdc, RGB(255, 255, 255));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Set background color
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        HBRUSH hbrBackground = CreateSolidBrush(APP_BG_COLOR);
        FillRect(hdc, &rcClient, hbrBackground);
        DeleteObject(hbrBackground);
        
        // Add visual styling if needed
        if (app) {
            // Create a rounded rectangle for the entire client area
            RECT rcRounded = rcClient;
            InflateRect(&rcRounded, -5, -5);
            app->drawRoundedRect(hdc, rcRounded, APP_HIGHLIGHT_COLOR, 15);
        }
        
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);

        if (id == 1 && event == CBN_SELCHANGE) // Sound pack combobox
        {
            int selectedIndex = SendMessage(app->comboBox_, CB_GETCURSEL, 0, 0);
            if (selectedIndex != CB_ERR && selectedIndex < static_cast<int>(app->soundPacks_.size()))
            {
                app->updateSoundPack(app->soundPacks_[selectedIndex]);
            }
        }
        else if (id == 7 && event == CBN_SELCHANGE) // Optimization combobox
        {
            int selectedLevel = SendMessage(app->optimizationCombo_, CB_GETCURSEL, 0, 0);
            if (selectedLevel != CB_ERR && selectedLevel >= 0 && selectedLevel <= 3)
            {
                app->setLatencyOptimization(selectedLevel);
            }
        }
        return 0;
    }

    case WM_HSCROLL: // Volume slider
    {
        if (app && reinterpret_cast<HWND>(lParam) == app->volumeSlider_)
        {
            int newVolume = SendMessage(app->volumeSlider_, TBM_GETPOS, 0, 0);
            app->setVolume(newVolume);
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
        
    case WM_ERASEBKGND:
        return 1; // Skip default erase to prevent flicker
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Application::SetControlColors(HWND hwnd)
{
    // Set colors for comboboxes
    if (comboBox_) {
        SendMessage(comboBox_, CB_SETCURSEL, 0, 0);
    }
    
    if (optimizationCombo_) {
        SendMessage(optimizationCombo_, CB_SETCURSEL, latencyOptimizationLevel_, 0);
    }
    
    // Set volume text
    if (volumeValueLabel_) {
        wchar_t volumeText[16];
        swprintf_s(volumeText, L"%d%%", volume_);
        SetWindowTextW(volumeValueLabel_, volumeText);
    }
}