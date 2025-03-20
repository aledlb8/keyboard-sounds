#include "KeyboardHookManager.h"
#include <iostream>
#include <unordered_set>

static std::unordered_set<UINT> keysDown;

KeyboardHookManager *KeyboardHookManager::instance = nullptr;

KeyboardHookManager::KeyboardHookManager(SoundManager &sm, MciSoundPlayer &sp)
    : soundManager(sm), soundPlayer(sp), hook(nullptr)
{
    instance = this;
}

KeyboardHookManager::~KeyboardHookManager()
{
    uninstallHook();
    instance = nullptr;
}

bool KeyboardHookManager::installHook()
{
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, nullptr, 0);
    if (!hook)
    {
        std::cerr << "Failed to install keyboard hook." << std::endl;
        return false;
    }
    return true;
}

void KeyboardHookManager::uninstallHook()
{
    if (hook)
    {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
    }
}

void KeyboardHookManager::setSelectedSound(const std::string &sound)
{
    selectedSound = sound;
}

LRESULT CALLBACK KeyboardHookManager::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *pKey = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (wParam == WM_KEYDOWN)
        {
            // If the key is already pressed, ignore this event.
            if (keysDown.find(pKey->vkCode) != keysDown.end())
            {
                return CallNextHookEx(instance ? instance->hook : nullptr, nCode, wParam, lParam);
            }
            // Mark key as pressed.
            keysDown.insert(pKey->vkCode);
            // Trigger sound for key-down.
            if (instance)
            {
                std::string soundFile = instance->soundManager.getRandomSoundForKey(pKey->vkCode, true);
                if (!soundFile.empty())
                {
                    instance->soundPlayer.playSound(soundFile);
                }
            }
        }
        else if (wParam == WM_KEYUP)
        {
            // Remove key from pressed set.
            keysDown.erase(pKey->vkCode);
            // Optionally, trigger a key-up sound.
            if (instance)
            {
                std::string soundFile = instance->soundManager.getRandomSoundForKey(pKey->vkCode, false);
                if (!soundFile.empty())
                {
                    instance->soundPlayer.playSound(soundFile);
                }
            }
        }
    }
    return CallNextHookEx(instance ? instance->hook : nullptr, nCode, wParam, lParam);
}