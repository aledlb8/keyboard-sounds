/**
 * @file SoundManager.h
 * @brief Manages sound files and categories
 */
#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

/**
 * @struct SoundCategory
 * @brief Structure for a sound category with separate "down" and "up" sounds
 */
struct SoundCategory
{
    std::vector<std::string> down; ///< List of sound files for key down events
    std::vector<std::string> up;   ///< List of sound files for key up events
};

/**
 * @enum KeyType
 * @brief Types of keys that have specialized sounds
 */
enum class KeyType
{
    ALPHA, ///< Regular alphanumeric keys
    ALT,   ///< Alt key
    ENTER, ///< Enter key
    SPACE, ///< Space key
    OTHER  ///< Any other key
};

/**
 * @class SoundManager
 * @brief Manages loading and retrieving sound files for keyboard events
 */
class SoundManager
{
public:
    /**
     * @brief Constructor
     * @param folder Path to the base sounds folder
     */
    explicit SoundManager(const std::string &folder);

    /**
     * @brief Destructor
     */
    ~SoundManager() = default;

    /**
     * @brief Load sounds from the current folder path
     * @return true if successful, false otherwise
     */
    bool loadSounds();

    /**
     * @brief Get a random sound file for a specific key event
     * @param vkCode Virtual key code of the key
     * @param keyDown true for key down event, false for key up
     * @return Path to the sound file or empty string if none found
     */
    std::string getRandomSoundForKey(WORD vkCode, bool keyDown) const;

    /**
     * @brief Set a new folder path for sounds
     * @param newFolder Path to the new folder
     */
    void setFolderPath(const std::string &newFolder);

    /**
     * @brief Get the current folder path
     * @return Current folder path
     */
    const std::string &getFolderPath() const;

    /**
     * @brief Add a custom key mapping
     * @param vkCode Virtual key code to map
     * @param type Key type to associate with this key
     */
    void addKeyMapping(WORD vkCode, KeyType type);

private:
    /**
     * @brief Load sounds for a specific category
     * @param categoryName Name of the category folder
     * @param cat SoundCategory to populate
     * @return true if successful, false otherwise
     */
    bool loadSoundCategory(const std::string &categoryName, SoundCategory &cat);

    /**
     * @brief Get the key type for a given virtual key code
     * @param vkCode Virtual key code
     * @return KeyType for the given key
     */
    KeyType getKeyTypeForVkCode(WORD vkCode) const;

    // Data members
    std::string folderPath_;

    std::unordered_map<KeyType, SoundCategory> categories_;
    std::unordered_map<WORD, KeyType> keyMappings_;
};

#endif // SOUNDMANAGER_H