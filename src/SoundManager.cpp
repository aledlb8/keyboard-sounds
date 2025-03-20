#include "SoundManager.h"
#include <filesystem>
#include <iostream>

SoundManager::SoundManager(const std::string &folder) : folderPath(folder) {}

bool SoundManager::loadSoundCategory(const std::string &categoryName, SoundCategory &cat)
{
    std::string downPath = folderPath + "/" + categoryName + "/down";
    std::string upPath = folderPath + "/" + categoryName + "/up";
    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(downPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".mp3")
            {
                cat.down.push_back(entry.path().string());
            }
        }
        for (const auto &entry : std::filesystem::directory_iterator(upPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".mp3")
            {
                cat.up.push_back(entry.path().string());
            }
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Error loading category '" << categoryName << "': " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool SoundManager::loadSounds()
{
    // Clear any previously loaded sounds.
    alpha.down.clear();
    alpha.up.clear();
    alt.down.clear();
    alt.up.clear();
    enter.down.clear();
    enter.up.clear();
    space.down.clear();
    space.up.clear();

    bool ok = true;
    ok = loadSoundCategory("alpha", alpha) && ok;
    ok = loadSoundCategory("alt", alt) && ok;
    ok = loadSoundCategory("enter", enter) && ok;
    ok = loadSoundCategory("space", space) && ok;
    return ok;
}

std::string SoundManager::getRandomSoundForKey(WORD vkCode, bool keyDown) const
{
    const std::vector<std::string> *vec = nullptr;
    if (vkCode == VK_SPACE)
    {
        vec = keyDown ? &space.down : &space.up;
    }
    else if (vkCode == VK_RETURN)
    {
        vec = keyDown ? &enter.down : &enter.up;
    }
    else if (vkCode == VK_MENU)
    { // Alt key.
        vec = keyDown ? &alt.down : &alt.up;
    }
    else
    {
        // For every other key, use the alpha category.
        vec = keyDown ? &alpha.down : &alpha.up;
    }
    if (vec && !vec->empty())
    {
        int index = rand() % vec->size();
        return (*vec)[index];
    }
    return "";
}

const std::string &SoundManager::getFolderPath() const
{
    return folderPath;
}