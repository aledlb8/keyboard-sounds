#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <windows.h> // Add this line to define WORD
#include <string>
#include <vector>

// New structure for a sound category with separate "down" and "up" sounds.
struct SoundCategory
{
    std::vector<std::string> down;
    std::vector<std::string> up;
};

class SoundManager
{
public:
    SoundManager(const std::string &folder);
    bool loadSounds();
    std::string getRandomSoundForKey(WORD vkCode, bool keyDown) const;
    void setFolderPath(const std::string &newFolder) { folderPath = newFolder; }
    const std::string &getFolderPath() const;

private:
    std::string folderPath;
    SoundCategory alpha;
    SoundCategory alt;
    SoundCategory enter;
    SoundCategory space;

    bool loadSoundCategory(const std::string &categoryName, SoundCategory &cat);
};

#endif // SOUNDMANAGER_H