/**
 * @file KeyboardHookManager.h
 * @brief Manages keyboard hooks and event handling
 */
#ifndef KEYBOARDHOOKMANAGER_H
#define KEYBOARDHOOKMANAGER_H

#include <windows.h>
#include <string>
#include <unordered_set>
#include <memory>
#include <functional>
#include <deque>
#include <unordered_map>

// Forward declarations
class SoundManager;
class SFMLSoundPlayer;

/**
 * @class KeyboardHookManager
 * @brief Manages Windows keyboard hooks to detect key events and play corresponding sounds
 */
class KeyboardHookManager
{
public:
    /**
     * @brief Constructor
     * @param soundManager Reference to the sound manager
     * @param soundPlayer Reference to the sound player
     */
    KeyboardHookManager(SoundManager &soundManager, SFMLSoundPlayer &soundPlayer);

    /**
     * @brief Destructor
     * Ensures the hook is uninstalled
     */
    ~KeyboardHookManager();

    /**
     * @brief Deleted copy constructor
     */
    KeyboardHookManager(const KeyboardHookManager &) = delete;

    /**
     * @brief Deleted assignment operator
     */
    KeyboardHookManager &operator=(const KeyboardHookManager &) = delete;

    /**
     * @brief Install the keyboard hook
     * @return true if successful, false otherwise
     */
    bool installHook();

    /**
     * @brief Uninstall the keyboard hook
     */
    void uninstallHook();

    /**
     * @brief Set an option to filter specific keys
     * @param enabled Whether key filtering is enabled
     */
    void setKeyFilteringEnabled(bool enabled);

    /**
     * @brief Add a key to the filter list
     * @param vkCode Virtual key code to add
     */
    void addKeyToFilter(WORD vkCode);

    /**
     * @brief Remove a key from the filter list
     * @param vkCode Virtual key code to remove
     */
    void removeKeyFromFilter(WORD vkCode);
    
    /**
     * @brief Set latency optimization level
     * @param level Optimization level (0-3, where 3 is maximum)
     */
    void setLatencyOptimization(int level);

private:
    /**
     * @brief Preload sounds for commonly used keys
     */
    void preloadCommonSounds();
    
    /**
     * @brief Preload sounds for likely key combinations
     * @param baseKey The key that was just pressed
     */
    void preloadPredictedKeys(WORD baseKey);
    
    /**
     * @brief Windows keyboard hook procedure
     * @param nCode Hook code
     * @param wParam Message identifier
     * @param lParam Pointer to keyboard event data
     * @return Result of the hook chain
     */
    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Process a key down event
     * @param vkCode Virtual key code
     */
    void handleKeyDown(WORD vkCode);

    /**
     * @brief Process a key up event
     * @param vkCode Virtual key code
     */
    void handleKeyUp(WORD vkCode);

    /**
     * @brief Check if a key should be processed
     * @param vkCode Virtual key code
     * @return true if the key should be processed, false otherwise
     */
    bool shouldProcessKey(WORD vkCode) const;

    // References to dependent objects
    SoundManager &soundManager_;
    SFMLSoundPlayer &soundPlayer_;

    // Windows hook handle
    HHOOK hook_;

    // Key filtering
    bool keyFilteringEnabled_;
    std::unordered_set<WORD> filteredKeys_;

    // Performance optimization level
    int latencyOptimizationLevel_;

    // Currently pressed keys
    static std::unordered_set<WORD> pressedKeys_;

    // Singleton instance for hook callback
    static KeyboardHookManager *instance_;
};

#endif // KEYBOARDHOOKMANAGER_H