#ifndef MCISOUNDPLAYER_H
#define MCISOUNDPLAYER_H

#include <string>

class MciSoundPlayer
{
public:
    MciSoundPlayer();
    void playSound(const std::string &filePath);
};

#endif // MCISOUNDPLAYER_H