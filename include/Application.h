#ifndef APPLICATION_H
#define APPLICATION_H

#include "SoundManager.h"
#include "MciSoundPlayer.h"
#include "KeyboardHookManager.h"

class Application
{
public:
    Application(const std::string &soundFolder);
    int run();
    void updateSoundPack(const std::string &pack);

private:
    SoundManager soundManager;
    MciSoundPlayer soundPlayer;
    KeyboardHookManager hookManager;
};

#endif // APPLICATION_H