/**
 * @file SFMLSoundPlayer.h
 * @brief Modern audio player component using SFML library
 */
#ifndef SFMLSOUNDPLAYER_H
#define SFMLSOUNDPLAYER_H

#include <string>
#include <mutex>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <SFML/Audio.hpp>
#include <queue>
#include <thread>
#include <chrono>
#include <vector>
#include <future>

/**
 * @class SFMLSoundPlayer
 * @brief Plays audio files using SFML Audio library
 *
 * This class provides a thread-safe way to play audio files
 * with volume control and automatic resource cleanup.
 * It uses modern C++ features and SFML for better performance
 * and lower latency than the older MCI system.
 */
class SFMLSoundPlayer
{
public:
    /**
     * @brief Constructor
     */
    SFMLSoundPlayer();

    /**
     * @brief Destructor
     * Ensures all sound resources are properly released
     */
    ~SFMLSoundPlayer();

    /**
     * @brief Deleted copy constructor
     */
    SFMLSoundPlayer(const SFMLSoundPlayer &) = delete;

    /**
     * @brief Deleted assignment operator
     */
    SFMLSoundPlayer &operator=(const SFMLSoundPlayer &) = delete;

    /**
     * @brief Play a sound file
     * @param filePath Path to the sound file
     * @param highPriority Whether the sound should be played with high priority
     * @return true if successful, false otherwise
     */
    bool playSound(const std::string &filePath, bool highPriority = false);

    /**
     * @brief Preloads a sound into the cache
     * @param filePath Path to the sound file to preload
     * @param highPriority Whether this is a high priority preload
     * @return true if successful, false otherwise
     */
    bool preloadSound(const std::string &filePath, bool highPriority = false);

    /**
     * @brief Set the global volume level
     * @param volume Volume level (0-100)
     */
    void setVolume(int volume);

    /**
     * @brief Get the current volume level
     * @return Current volume (0-100)
     */
    int getVolume() const;

    /**
     * @brief Stop all currently playing sounds
     */
    void stopAllSounds();

private:
    /**
     * @brief Process function for the sound queue thread
     */
    void processSoundQueue();
    
    /**
     * @brief Clean up finished sounds
     */
    void cleanupFinishedSounds();

    // Thread safety
    std::mutex soundsMutex_;
    std::mutex queueMutex_;
    std::mutex cacheMutex_;

    // Internal state
    std::atomic<int> volume_;
    std::atomic<bool> running_;
    
    // Sound processing thread
    std::thread processingThread_;
    
    /**
     * @brief Structure to hold a pending sound with priority
     */
    struct PendingSound {
        std::string path;
        bool highPriority;
        
        PendingSound(const std::string& p, bool hp) : path(p), highPriority(hp) {}
    };
    
    // Queue for pending sounds to play
    std::deque<PendingSound> pendingSounds_;
    
    // Sound buffers cache
    std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> soundBuffers_;
    
    // Currently playing sounds
    struct SoundInstance {
        std::shared_ptr<sf::Sound> sound;
        std::chrono::steady_clock::time_point expirationTime;
        std::string path;
        bool highPriority;
    };
    
    std::vector<SoundInstance> activeSounds_;
    
    // Store futures from preload operations to prevent warning about discarding them
    std::vector<std::future<void>> preloadFutures_;
    
    // Constants
    static constexpr int MAX_CONCURRENT_SOUNDS = 32; // SFML can handle more concurrent sounds
    static constexpr int MAX_CACHE_SIZE = 100;       // More generous cache size
    static constexpr auto CLEANUP_INTERVAL = std::chrono::seconds(1);
};

#endif // SFMLSOUNDPLAYER_H 