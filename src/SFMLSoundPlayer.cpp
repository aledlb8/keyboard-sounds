/**
 * @file SFMLSoundPlayer.cpp
 * @brief Implementation of the SFMLSoundPlayer class
 */
#include "SFMLSoundPlayer.h"
#include <iostream>
#include <algorithm>
#include <future>

SFMLSoundPlayer::SFMLSoundPlayer()
    : volume_(50),
      running_(true)
{
    // Start the sound processing thread
    processingThread_ = std::thread(&SFMLSoundPlayer::processSoundQueue, this);
}

SFMLSoundPlayer::~SFMLSoundPlayer()
{
    // Signal the processing thread to stop
    running_ = false;
    
    // Wait for the thread to finish
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
    
    // Stop and clear all sounds
    stopAllSounds();
    
    // Clear cache
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        soundBuffers_.clear();
    }
}

bool SFMLSoundPlayer::playSound(const std::string &filePath, bool highPriority)
{
    if (filePath.empty()) {
        return false;
    }
    
    // Add to the pending sounds queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        // Create a pending sound with priority information
        PendingSound pendingSound(filePath, highPriority);
        
        // High priority sounds go to the front of the queue
        if (highPriority) {
            // Insert high priority items at the front, but after any existing high priority items
            auto it = pendingSounds_.begin();
            while (it != pendingSounds_.end() && it->highPriority) {
                ++it;
            }
            pendingSounds_.insert(it, pendingSound);
        } else {
            // Low priority sounds go to the back
            pendingSounds_.push_back(pendingSound);
        }
        
        // Limit queue size to prevent memory issues during very fast typing
        if (pendingSounds_.size() > 64) {
            // Remove low priority sounds first
            auto it = std::find_if(pendingSounds_.begin(), pendingSounds_.end(), 
                [](const PendingSound& sound) { return !sound.highPriority; });
                
            if (it != pendingSounds_.end()) {
                pendingSounds_.erase(it);
            } else if (!pendingSounds_.empty()) {
                // If no low priority sounds, remove the oldest high priority sound
                pendingSounds_.pop_back();
            }
        }
    }
    
    return true;
}

bool SFMLSoundPlayer::preloadSound(const std::string &filePath, bool highPriority)
{
    if (filePath.empty()) {
        return false;
    }
    
    // Check if already in cache
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        if (soundBuffers_.find(filePath) != soundBuffers_.end()) {
            return true; // Already cached
        }
    }
    
    // For high priority preloads, load synchronously to ensure immediate availability
    if (highPriority) {
        auto buffer = std::make_shared<sf::SoundBuffer>();
        if (buffer->loadFromFile(filePath)) {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            // Check cache size
            if (soundBuffers_.size() >= MAX_CACHE_SIZE) {
                if (!soundBuffers_.empty()) {
                    soundBuffers_.erase(soundBuffers_.begin());
                }
            }
            soundBuffers_[filePath] = buffer;
            return true;
        } else {
            std::cerr << "Failed to preload sound file: " << filePath << std::endl;
            return false;
        }
    }
    
    // Low priority preloads can be done asynchronously
    auto future = std::async(std::launch::async, [this, filePath]() {
        auto buffer = std::make_shared<sf::SoundBuffer>();
        if (buffer->loadFromFile(filePath)) {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            // Check cache size
            if (soundBuffers_.size() >= MAX_CACHE_SIZE) {
                if (!soundBuffers_.empty()) {
                    soundBuffers_.erase(soundBuffers_.begin());
                }
            }
            soundBuffers_[filePath] = buffer;
        } else {
            std::cerr << "Failed to preload sound file: " << filePath << std::endl;
        }
    });
    
    // Store the future to prevent the warning about discarding it
    std::lock_guard<std::mutex> lock(queueMutex_); // Reuse an existing mutex
    preloadFutures_.push_back(std::move(future));
    
    // Clean up completed futures to avoid memory buildup
    preloadFutures_.erase(
        std::remove_if(preloadFutures_.begin(), preloadFutures_.end(),
            [](const std::future<void>& f) {
                return f.valid() && 
                       f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }),
        preloadFutures_.end());
    
    return true;
}

void SFMLSoundPlayer::processSoundQueue()
{
    auto lastCleanupTime = std::chrono::steady_clock::now();
    
    while (running_) {
        // Process pending sounds
        PendingSound soundToPlay("", false);
        bool hasSound = false;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (!pendingSounds_.empty()) {
                soundToPlay = pendingSounds_.front();
                pendingSounds_.pop_front();
                hasSound = true;
            }
        }
        
        if (hasSound) {
            // First check if we already have too many sounds playing
            {
                std::lock_guard<std::mutex> lock(soundsMutex_);
                if (activeSounds_.size() >= MAX_CONCURRENT_SOUNDS) {
                    // If this is a high priority sound, remove a low priority sound to make room
                    if (soundToPlay.highPriority) {
                        // Find a low priority sound to remove
                        auto it = std::find_if(activeSounds_.begin(), activeSounds_.end(),
                            [](const SoundInstance& instance) { return !instance.highPriority; });
                            
                        if (it != activeSounds_.end()) {
                            // Found a low priority sound to stop
                            it->sound->stop();
                            activeSounds_.erase(it);
                        } else {
                            // Remove the oldest sound if no low priority sounds are found
                            if (!activeSounds_.empty()) {
                                activeSounds_.erase(activeSounds_.begin());
                            }
                        }
                    } else {
                        // For low priority sounds, just skip if we're at capacity
                        continue;
                    }
                }
            }
            
            // Check if buffer is in cache
            std::shared_ptr<sf::SoundBuffer> buffer;
            bool bufferFound = false;
            
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                auto it = soundBuffers_.find(soundToPlay.path);
                if (it != soundBuffers_.end()) {
                    buffer = it->second;
                    bufferFound = true;
                }
            }
            
            // If not in cache, load it
            if (!bufferFound) {
                buffer = std::make_shared<sf::SoundBuffer>();
                if (!buffer->loadFromFile(soundToPlay.path)) {
                    std::cerr << "Failed to load sound file: " << soundToPlay.path << std::endl;
                    continue;
                }
                
                // Add to cache
                {
                    std::lock_guard<std::mutex> lock(cacheMutex_);
                    // Clean up cache if needed
                    if (soundBuffers_.size() >= MAX_CACHE_SIZE) {
                        // Simple strategy: just remove a random entry
                        if (!soundBuffers_.empty()) {
                            soundBuffers_.erase(soundBuffers_.begin());
                        }
                    }
                    
                    soundBuffers_[soundToPlay.path] = buffer;
                }
            }
            
            // Create sound instance (SFML 3 requires a buffer for construction)
            std::shared_ptr<sf::Sound> sound = std::make_shared<sf::Sound>(*buffer);
            sound->setVolume(static_cast<float>(volume_));
            sound->play();
            
            // Calculate expiration time (duration of sound + small buffer)
            auto duration = std::chrono::milliseconds(
                static_cast<int>(buffer->getDuration().asMilliseconds()) + 200);
            auto expiration = std::chrono::steady_clock::now() + duration;
            
            // Add to active sounds
            {
                std::lock_guard<std::mutex> lock(soundsMutex_);
                activeSounds_.push_back({sound, expiration, soundToPlay.path, soundToPlay.highPriority});
            }
        }
        
        // Periodically clean up finished sounds
        auto now = std::chrono::steady_clock::now();
        if (now - lastCleanupTime >= CLEANUP_INTERVAL) {
            cleanupFinishedSounds();
            lastCleanupTime = now;
        }
        
        // Sleep a bit to avoid consuming too much CPU
        // Use shorter sleep time for better responsiveness
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void SFMLSoundPlayer::cleanupFinishedSounds()
{
    std::lock_guard<std::mutex> lock(soundsMutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    // Remove finished sounds
    activeSounds_.erase(
        std::remove_if(activeSounds_.begin(), activeSounds_.end(),
            [&now](const SoundInstance& instance) {
                // Check if sound is finished or expired
                return instance.sound->getStatus() == sf::Sound::Status::Stopped ||
                       now >= instance.expirationTime;
            }),
        activeSounds_.end()
    );
}

void SFMLSoundPlayer::setVolume(int volume)
{
    // Clamp volume between 0-100
    volume_ = std::clamp(volume, 0, 100);
    
    // Update volume for all active sounds
    std::lock_guard<std::mutex> lock(soundsMutex_);
    for (auto& instance : activeSounds_) {
        instance.sound->setVolume(static_cast<float>(volume_));
    }
}

int SFMLSoundPlayer::getVolume() const
{
    return volume_;
}

void SFMLSoundPlayer::stopAllSounds()
{
    // Clear pending sounds queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pendingSounds_.clear();
    }
    
    // Stop all active sounds
    {
        std::lock_guard<std::mutex> lock(soundsMutex_);
        for (auto& instance : activeSounds_) {
            instance.sound->stop();
        }
        activeSounds_.clear();
    }
} 