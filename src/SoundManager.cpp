/**
 * @file SoundManager.cpp
 * @brief Implementation of the SoundManager class
 */
#include "SoundManager.h"
#include "Utils.h"
#include <filesystem>
#include <iostream>
#include <random>
#include <algorithm>

SoundManager::SoundManager(const std::string &folder) : folderPath_(folder)
{
    // Initialize key mappings
    keyMappings_[VK_SPACE] = KeyType::SPACE;
    keyMappings_[VK_RETURN] = KeyType::ENTER;
    keyMappings_[VK_MENU] = KeyType::ALT; // Alt key

    // Initialize categories map
    categories_[KeyType::ALPHA] = SoundCategory();
    categories_[KeyType::ALT] = SoundCategory();
    categories_[KeyType::ENTER] = SoundCategory();
    categories_[KeyType::SPACE] = SoundCategory();
    categories_[KeyType::OTHER] = SoundCategory();
}

bool SoundManager::loadSoundCategory(const std::string &categoryName, SoundCategory &cat)
{
    std::string downPath = folderPath_ + "/" + categoryName + "/down";
    std::string upPath = folderPath_ + "/" + categoryName + "/up";

    // Clear existing sounds
    cat.down.clear();
    cat.up.clear();

    bool foundFiles = false;

    try
    {
        // Load down sounds
        if (std::filesystem::exists(downPath) && std::filesystem::is_directory(downPath))
        {
            for (const auto &entry : std::filesystem::directory_iterator(downPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".mp3")
                {
                    cat.down.push_back(entry.path().string());
                    foundFiles = true;
                }
            }
        }
        else
        {
            std::cerr << "Directory not found or not accessible: " << downPath << std::endl;
        }

        // Load up sounds
        if (std::filesystem::exists(upPath) && std::filesystem::is_directory(upPath))
        {
            for (const auto &entry : std::filesystem::directory_iterator(upPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".mp3")
                {
                    cat.up.push_back(entry.path().string());
                    foundFiles = true;
                }
            }
        }
        else
        {
            std::cerr << "Directory not found or not accessible: " << upPath << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Error loading category '" << categoryName << "': " << e.what() << std::endl;
        return false;
    }

    return foundFiles;
}

bool SoundManager::loadSounds()
{
    // Print the current folder path for debugging
    std::cerr << "Loading sounds from: " << folderPath_ << std::endl;
    
    // Check if the path exists
    if (!std::filesystem::exists(folderPath_))
    {
        std::cerr << "Error: Sound pack directory does not exist: " << folderPath_ << std::endl;
        return false;
    }
    
    // Clear all categories
    for (auto &[type, category] : categories_)
    {
        category.down.clear();
        category.up.clear();
    }

    // Map from KeyType to category name
    const std::unordered_map<KeyType, std::string> categoryNames = {
        {KeyType::ALPHA, "alpha"},
        {KeyType::ALT, "alt"},
        {KeyType::ENTER, "enter"},
        {KeyType::SPACE, "space"},
        {KeyType::OTHER, "other"}};

    bool anySuccess = false;

    // Load each category
    for (const auto &[type, name] : categoryNames)
    {
        bool result = loadSoundCategory(name, categories_[type]);
        anySuccess |= result;
        std::cerr << "Loading category '" << name << "': " << (result ? "success" : "failed") << std::endl;
    }

    // If alpha category is empty, try to load a fallback
    if (categories_[KeyType::ALPHA].down.empty() && categories_[KeyType::ALPHA].up.empty())
    {
        // Use "other" category as fallback if it exists
        if (!categories_[KeyType::OTHER].down.empty() || !categories_[KeyType::OTHER].up.empty())
        {
            categories_[KeyType::ALPHA] = categories_[KeyType::OTHER];
            std::cerr << "Using 'other' category as fallback for 'alpha'" << std::endl;
        }
    }

    return anySuccess;
}

std::string SoundManager::getRandomSoundForKey(WORD vkCode, bool keyDown) const
{
    // Get the key type for this virtual key code
    KeyType keyType = getKeyTypeForVkCode(vkCode);

    // Get the appropriate sound category
    auto it = categories_.find(keyType);
    if (it == categories_.end())
    {
        // Fallback to ALPHA category
        it = categories_.find(KeyType::ALPHA);
        if (it == categories_.end())
        {
            // No sounds available
            return "";
        }
    }

    // Get the appropriate sound list (down or up)
    const std::vector<std::string> &sounds = keyDown ? it->second.down : it->second.up;

    // Return a random sound
    if (!sounds.empty())
    {
        // Use a proper random number generator
        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::uniform_int_distribution<> distrib(0, static_cast<int>(sounds.size()) - 1);
        return sounds[distrib(gen)];
    }

    return "";
}

void SoundManager::setFolderPath(const std::string &newFolder)
{
    folderPath_ = newFolder;
}

const std::string &SoundManager::getFolderPath() const
{
    return folderPath_;
}

void SoundManager::addKeyMapping(WORD vkCode, KeyType type)
{
    keyMappings_[vkCode] = type;
}

KeyType SoundManager::getKeyTypeForVkCode(WORD vkCode) const
{
    auto it = keyMappings_.find(vkCode);
    if (it != keyMappings_.end())
    {
        return it->second;
    }

    // Default to ALPHA for most keys
    return KeyType::ALPHA;
}