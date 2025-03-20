#include "MciSoundPlayer.h"
#include <windows.h>
#include <mmsystem.h>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>
#include <cstdlib>

// Declare external volume variable.
extern int g_volume;

#pragma comment(lib, "winmm.lib")

MciSoundPlayer::MciSoundPlayer() {}

void MciSoundPlayer::playSound(const std::string &filePath)
{
    static std::mutex aliasMutex;
    std::lock_guard<std::mutex> lock(aliasMutex);
    static int aliasCount = 0;
    std::stringstream aliasStream;
    aliasStream << "mp3_" << aliasCount++;
    std::string alias = aliasStream.str();

    std::string openCmd = "open \"" + filePath + "\" type mpegvideo alias " + alias;
    if (mciSendString(openCmd.c_str(), nullptr, 0, nullptr) != 0)
    {
        std::cerr << "Failed to open sound file: " << filePath << std::endl;
        return;
    }

    // Set volume. Map g_volume (0-100) to a scale acceptable to MCI.
    // Often volume is in the range 0-1000.
    int mciVolume = g_volume * 10;
    std::string volumeCmd = "setaudio " + alias + " volume to " + std::to_string(mciVolume);
    mciSendString(volumeCmd.c_str(), nullptr, 0, nullptr);

    std::string playCmd = "play " + alias;
    if (mciSendString(playCmd.c_str(), nullptr, 0, nullptr) != 0)
    {
        std::cerr << "Failed to play sound file: " << filePath << std::endl;
        mciSendString(("close " + alias).c_str(), nullptr, 0, nullptr);
        return;
    }

    char lengthBuffer[128] = {0};
    std::string statusCmd = "status " + alias + " length";
    mciSendString(statusCmd.c_str(), lengthBuffer, sizeof(lengthBuffer), nullptr);
    int length = atoi(lengthBuffer);
    if (length <= 0)
    {
        length = 5000;
    }
    std::thread([alias, length]()
                {
        std::this_thread::sleep_for(std::chrono::milliseconds(length + 100));
        std::string closeCmd = "close " + alias;
        mciSendString(closeCmd.c_str(), nullptr, 0, nullptr); })
        .detach();
}