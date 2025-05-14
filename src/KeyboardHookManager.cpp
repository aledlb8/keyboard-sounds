/**
 * @file KeyboardHookManager.cpp
 * @brief Implementation of the KeyboardHookManager class
 */
#include "KeyboardHookManager.h"
#include "SoundManager.h"
#include "SFMLSoundPlayer.h"
#include <iostream>
#include <unordered_set>
#include <chrono>
#include <unordered_map>
#include <deque>
#include <thread>
#include <future>
#include <algorithm>

// Initialize static members
KeyboardHookManager *KeyboardHookManager::instance_ = nullptr;
std::unordered_set<WORD> KeyboardHookManager::pressedKeys_;

// For fast typing - keep track of key timings
static std::unordered_map<WORD, std::chrono::steady_clock::time_point> keyTimestamps;
static constexpr auto KEY_PROCESSING_INTERVAL = std::chrono::milliseconds(25); // Reduced from 40ms for lower latency

// Predictive cache - keep track of common key sequences to prefetch sounds
static std::deque<WORD> recentKeys;
static constexpr size_t KEY_HISTORY_LENGTH = 5;
static std::unordered_map<WORD, std::unordered_set<WORD>> keyFollowers;

KeyboardHookManager::KeyboardHookManager(SoundManager &soundManager, SFMLSoundPlayer &soundPlayer)
    : soundManager_(soundManager),
      soundPlayer_(soundPlayer),
      hook_(nullptr),
      keyFilteringEnabled_(false),
      latencyOptimizationLevel_(2) // Default to medium optimization
{
    // Set the singleton instance for the hook callback
    if (instance_ != nullptr)
    {
        std::cerr << "Warning: Multiple KeyboardHookManager instances created." << std::endl;
    }
    instance_ = this;
    
    // Preload common keys in advance
    preloadCommonSounds();
}

void KeyboardHookManager::preloadCommonSounds()
{
    // Preload sounds for common keys with high priority
    const std::vector<WORD> commonKeys = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        VK_SPACE, VK_RETURN, VK_BACK, VK_TAB, VK_LSHIFT, VK_RSHIFT,
        VK_LCONTROL, VK_RCONTROL, VK_ESCAPE, VK_CAPITAL
    };
    
    // Preload by making requests to the sound manager and explicitly preloading
    for (WORD key : commonKeys)
    {
        std::string downSound = soundManager_.getRandomSoundForKey(key, true);
        std::string upSound = soundManager_.getRandomSoundForKey(key, false);
        
        if (!downSound.empty())
        {
            soundPlayer_.preloadSound(downSound, true); // High priority preload
        }
        
        if (!upSound.empty())
        {
            soundPlayer_.preloadSound(upSound, true); // High priority preload
        }
    }
}

void KeyboardHookManager::setLatencyOptimization(int level)
{
    // Clamp level to valid range (0-3)
    latencyOptimizationLevel_ = std::clamp(level, 0, 3);
    
    // Adjust behavior based on optimization level
    switch (latencyOptimizationLevel_) {
        case 0: // Minimum optimization
            // Don't preload predicted keys, use longer processing intervals
            keyFollowers.clear();
            recentKeys.clear();
            break;
            
        case 1: // Low optimization
            // Keep shorter history for predictions
            while (recentKeys.size() > 3) {
                recentKeys.pop_front();
            }
            break;
            
        case 2: // Medium optimization (default)
            // Default settings are fine
            break;
            
        case 3: // Maximum optimization
            // Prefetch more aggressively
            // Preload the most common keys again with high priority
            preloadCommonSounds();
            break;
    }
}

void KeyboardHookManager::preloadPredictedKeys(WORD baseKey)
{
    // Skip if optimization level is too low
    if (latencyOptimizationLevel_ < 1) {
        return;
    }
    
    // Preload the next likely keys based on learning history
    auto it = keyFollowers.find(baseKey);
    if (it != keyFollowers.end()) {
        // Number of keys to preload depends on optimization level
        size_t keysToPreload = latencyOptimizationLevel_;
        
        size_t count = 0;
        for (WORD nextKey : it->second) {
            if (count >= keysToPreload) break;
            
            // Preload with priority based on optimization level
            bool highPriority = (latencyOptimizationLevel_ >= 3);
            
            std::string nextDownSound = soundManager_.getRandomSoundForKey(nextKey, true);
            std::string nextUpSound = soundManager_.getRandomSoundForKey(nextKey, false);
            
            if (!nextDownSound.empty()) {
                soundPlayer_.preloadSound(nextDownSound, highPriority);
            }
            
            if (!nextUpSound.empty()) {
                soundPlayer_.preloadSound(nextUpSound, highPriority);
            }
            
            count++;
        }
    }
}

KeyboardHookManager::~KeyboardHookManager()
{
    uninstallHook();

    // Only clear the singleton if this instance is the current one
    if (instance_ == this)
    {
        instance_ = nullptr;
    }
}

bool KeyboardHookManager::installHook()
{
    // If a hook is already installed, uninstall it first
    if (hook_ != nullptr)
    {
        uninstallHook();
    }

    // Install the low-level keyboard hook
    hook_ = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, nullptr, 0);
    if (hook_ == nullptr)
    {
        DWORD error = GetLastError();
        std::cerr << "Failed to install keyboard hook. Error code: " << error << std::endl;
        return false;
    }

    return true;
}

void KeyboardHookManager::uninstallHook()
{
    if (hook_ != nullptr)
    {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;

        // Clear the pressed keys set
        pressedKeys_.clear();
    }
}

void KeyboardHookManager::setKeyFilteringEnabled(bool enabled)
{
    keyFilteringEnabled_ = enabled;
}

void KeyboardHookManager::addKeyToFilter(WORD vkCode)
{
    filteredKeys_.insert(vkCode);
}

void KeyboardHookManager::removeKeyFromFilter(WORD vkCode)
{
    filteredKeys_.erase(vkCode);
}

bool KeyboardHookManager::shouldProcessKey(WORD vkCode) const
{
    // If filtering is disabled, process all keys
    if (!keyFilteringEnabled_)
    {
        return true;
    }

    // If the key is in the filter list, don't process it
    return filteredKeys_.find(vkCode) == filteredKeys_.end();
}

void KeyboardHookManager::handleKeyDown(WORD vkCode)
{
    // Rate limiting for repeated keys during fast typing
    auto now = std::chrono::steady_clock::now();
    auto it = keyTimestamps.find(vkCode);
    
    bool shouldPlay = true;
    if (it != keyTimestamps.end())
    {
        auto timeSinceLastPress = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (timeSinceLastPress < KEY_PROCESSING_INTERVAL)
        {
            // Key pressed too quickly after last press, prioritize next key processing
            shouldPlay = false;
        }
    }
    
    // Update timestamp for this key
    keyTimestamps[vkCode] = now;
    
    // Update predictive cache - learn key sequences
    if (latencyOptimizationLevel_ > 0 && !recentKeys.empty())
    {
        WORD previousKey = recentKeys.back();
        keyFollowers[previousKey].insert(vkCode);
        
        // Preload sounds for keys that often follow the current key
        preloadPredictedKeys(vkCode);
    }
    
    // Add to recent keys and maintain fixed length
    if (latencyOptimizationLevel_ > 0) {
        recentKeys.push_back(vkCode);
        while (recentKeys.size() > KEY_HISTORY_LENGTH) {
            recentKeys.pop_front();
        }
    }
    
    // Play key down sound if we should
    if (shouldPlay)
    {
        std::string soundFile = soundManager_.getRandomSoundForKey(vkCode, true);
        if (!soundFile.empty())
        {
            // Play the sound with high priority
            soundPlayer_.playSound(soundFile, true);
        }
    }
}

void KeyboardHookManager::handleKeyUp(WORD vkCode)
{
    // Rate limiting for key up events during very fast typing
    auto now = std::chrono::steady_clock::now();
    auto it = keyTimestamps.find(vkCode);
    
    bool shouldPlay = true;
    if (it != keyTimestamps.end())
    {
        auto timeSinceKeyDown = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        
        // If the key was released extremely quickly (less than 20ms), we may not want to play the up sound
        // as this could indicate very fast typing where playing every sound would cause clipping
        if (timeSinceKeyDown < std::chrono::milliseconds(20))
        {
            // For very fast typing, we prioritize down sounds over up sounds
            shouldPlay = false;
        }
    }

    // Play key up sound if we should
    if (shouldPlay)
    {
        std::string soundFile = soundManager_.getRandomSoundForKey(vkCode, false);
        if (!soundFile.empty())
        {
            // Play with lower priority
            soundPlayer_.playSound(soundFile, false);
        }
    }
}

LRESULT CALLBACK KeyboardHookManager::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // We must call the next hook in the chain, even if we process the message
    if (nCode != HC_ACTION || instance_ == nullptr)
    {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *pKey = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
    WORD vkCode = pKey->vkCode;

    // Check if we should process this key
    if (!instance_->shouldProcessKey(vkCode))
    {
        return CallNextHookEx(instance_->hook_, nCode, wParam, lParam);
    }

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
    {
        // Get injected flag - bit 4 (0x10) in flags
        bool isInjected = (pKey->flags & 0x10) != 0;
        
        // Ignore injected keystrokes which might be from other software
        if (!isInjected)
        {
            // If the key is already pressed (key repeat), ignore this event
            if (pressedKeys_.find(vkCode) == pressedKeys_.end())
            {
                // Mark key as pressed
                pressedKeys_.insert(vkCode);

                // To minimize latency, handle directly in the hook thread for non-injected keystrokes
                // This trades some potential UI responsiveness for sound latency
                instance_->handleKeyDown(vkCode);
            }
        }
    }
    else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
    {
        // Get injected flag - bit 4 (0x10) in flags
        bool isInjected = (pKey->flags & 0x10) != 0;
        
        // Ignore injected keystrokes
        if (!isInjected)
        {
            // Remove key from pressed set
            pressedKeys_.erase(vkCode);

            // Handle key up event
            instance_->handleKeyUp(vkCode);
        }
    }

    // Pass the message to the next hook in the chain
    return CallNextHookEx(instance_->hook_, nCode, wParam, lParam);
}