/**
 * @file Application.h
 * @brief Main application class responsible for UI and program lifecycle
 */
#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>
#include <memory>
#include <vector>
#include <windows.h>
#include "SoundManager.h"
#include "SFMLSoundPlayer.h"
#include "KeyboardHookManager.h"

/**
 * @class Application
 * @brief Main application class that manages the lifecycle and UI
 */
class Application
{
public:
    /**
     * @brief Constructor
     * @param soundFolder Path to the folder containing sound packs
     */
    explicit Application(const std::string &soundFolder);

    /**
     * @brief Destructor
     */
    ~Application();

    /**
     * @brief Runs the application main loop
     * @return Exit code
     */
    int run();

    /**
     * @brief Updates the current sound pack
     * @param pack Path to the sound pack
     * @return true if successful, false otherwise
     */
    bool updateSoundPack(const std::string &pack);

    /**
     * @brief Sets the volume level
     * @param volume Volume level (0-100)
     */
    void setVolume(int volume);

    /**
     * @brief Gets the current volume level
     * @return Current volume (0-100)
     */
    int getVolume() const;
    
    /**
     * @brief Sets the latency optimization level
     * @param level Optimization level (0-3)
     * 0 = Minimal optimization (better compatibility)
     * 1 = Low optimization
     * 2 = Medium optimization (default)
     * 3 = Maximum optimization (lowest latency, but higher CPU usage)
     */
    void setLatencyOptimization(int level);

private:
    /**
     * @brief Window procedure callback, needed for WinAPI
     */
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Initializes the application window and UI elements
     * @return true if successful, false otherwise
     */
    bool initializeWindow();

    /**
     * @brief Loads available sound packs
     * @return true if at least one sound pack was found, false otherwise
     */
    bool loadSoundPacks();
    
    /**
     * @brief Creates a font with the specified properties
     * @param size Font size in points
     * @param bold Whether the font should be bold
     * @param italic Whether the font should be italic
     * @return HFONT handle to the created font
     */
    HFONT createFont(int size, bool bold, bool italic);
    
    /**
     * @brief Draws a rounded rectangle 
     * @param hdc Device context
     * @param rect Rectangle coordinates
     * @param color Fill color
     * @param radius Corner radius
     */
    void drawRoundedRect(HDC hdc, RECT rect, COLORREF color, int radius);
    
    /**
     * @brief Sets custom colors for controls
     * @param hwnd Window handle
     */
    void SetControlColors(HWND hwnd);

    // Data members
    std::string soundFolder_;
    std::vector<std::string> soundPacks_;

    std::unique_ptr<SoundManager> soundManager_;
    std::unique_ptr<SFMLSoundPlayer> soundPlayer_;
    std::unique_ptr<KeyboardHookManager> hookManager_;

    // UI elements
    HWND hwnd_;
    HWND comboBox_;
    HWND volumeSlider_;
    HWND volumeValueLabel_;
    HWND optimizationCombo_;
    
    // Resource management
    std::vector<HFONT> fonts_; // Store font handles for cleanup

    // Settings
    int volume_;
    int latencyOptimizationLevel_;

    // Window class name
    static constexpr const wchar_t *CLASS_NAME = L"KeyboardSoundsAppWindowClass";
    static constexpr int DEFAULT_VOLUME = 50;
    static constexpr int DEFAULT_OPTIMIZATION = 2;
};

#endif // APPLICATION_H