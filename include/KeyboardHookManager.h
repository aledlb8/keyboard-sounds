#ifndef KEYBOARDHOOKMANAGER_H
#define KEYBOARDHOOKMANAGER_H

#include <windows.h>
#include "SoundManager.h"
#include "MciSoundPlayer.h"

class KeyboardHookManager
{
public:
    KeyboardHookManager(SoundManager &sm, MciSoundPlayer &sp);
    ~KeyboardHookManager();
    bool installHook();
    void uninstallHook();

    void setSelectedSound(const std::string &sound);

    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    SoundManager &soundManager;
    MciSoundPlayer &soundPlayer;
    HHOOK hook;
    std::string selectedSound;
    static KeyboardHookManager *instance;
};

#endif // KEYBOARDHOOKMANAGER_H