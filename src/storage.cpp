#include "storage.h"
#include <chrono>
#include <climits>  // For LLONG_MAX
#include <cstdlib>  // For rand()
#include <ctime>    // For time()

// Check if value is expired
bool StoredValue::isExpired() const {
    if (expiresAt == -1) return false;
    return expiresAt <= Storage::getCurrentTimeMs();
}

// Get current time in milliseconds (Unix timestamp)
int64_t Storage::getCurrentTimeMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Set without expiration
void Storage::set(const string& key, const string& value) {
    evictIfNeeded();  // Evict before inserting if needed
    int64_t now = getCurrentTimeMs();
    data[key] = {value, -1, now};  // -1 = no expiration, now = lastAccessTime
}

// Set with expiration (DiceDB approach)
void Storage::setWithExpiry(const string& key, const string& value, int64_t durationMs) {
    evictIfNeeded();  // Evict before inserting if needed
    int64_t now = getCurrentTimeMs();
    int64_t expiresAt = -1;
    
    if (durationMs > 0) {
        expiresAt = now + durationMs;
    }
    
    // Deduce encoding for the value
    uint8_t encoding = deduceEncoding(value);
    data[key] = StoredValue(value, expiresAt, now, OBJ_TYPE_STRING | encoding);
}

// Get value with lazy expiration check
optional<string> Storage::get(const string& key) {
    auto it = data.find(key);
    
    // Key doesn't exist
    if (it == data.end()) {
        return nullopt;
    }
    
    // Check if expired (lazy deletion - Redis approach)
    if (it->second.isExpired()) {
        data.erase(it);  // Delete expired key from map
        return nullopt;
    }
    
    // Update lastAccessTime for LRU tracking
    it->second.lastAccessTime = getCurrentTimeMs();
    
    return it->second.value;
}

// Check existence with expiration check
bool Storage::exists(const string& key) {
    auto it = data.find(key);
    
    if (it == data.end()) {
        return false;
    }
    
    // Expired keys are treated as non-existent (lazy deletion)
    if (it->second.isExpired()) {
        data.erase(it);  // Delete expired key from map
        return false;
    }
    
    return true;
}

// Delete key
// void Storage::del(const string& key) {
//     data.erase(key);
// }

// Get TTL in seconds (DiceDB TTL command logic)
int64_t Storage::getTTL(const string& key) {
    auto it = data.find(key);
    
    // Key doesn't exist
    if (it == data.end()) {
        return -2;
    }
    
    // Check if expired (lazy deletion)
    if (it->second.isExpired()) {
        data.erase(it);  // Delete expired key from map
        return -2;  // Expired = doesn't exist
    }
    
    // No expiration set
    if (it->second.expiresAt == -1) {
        return -1;
    }
    
    // Calculate remaining time in seconds
    int64_t remainingMs = it->second.expiresAt - getCurrentTimeMs();
    return remainingMs / 1000;  // Convert to seconds
}

// Delete key (returns true if deleted, false if didn't exist)
bool Storage::del(const string& key) {
    if (data.find(key) != data.end()) {
        data.erase(key);
        return true;
    }
    return false;
}

// Set expiration on existing key (returns true if set, false if key doesn't exist)
bool Storage::expire(const string& key, int64_t durationSec) {
    auto it = data.find(key);
    
    // Key doesn't exist
    if (it == data.end()) {
        return false;
    }
    
    // Check if already expired
    if (it->second.isExpired()) {
        data.erase(it);
        return false;
    }
    
    // Set new expiration time
    int64_t durationMs = durationSec * 1000;  // Convert seconds to milliseconds
    it->second.expiresAt = getCurrentTimeMs() + durationMs;
    
    return true;
}

// Active expiration - delete expired keys using sampling (Redis/DiceDB approach)
void Storage::deleteExpiredKeys() {
    if (data.empty()) return;
    
    const int SAMPLE_SIZE = 20;
    const float THRESHOLD = 0.25f;  // 25%
    
    int sampled = 0;
    int expired = 0;
    int checked = 0;
    
    // Sample up to 20 keys with expiration set
    for (auto it = data.begin(); it != data.end() && sampled < SAMPLE_SIZE; ) {
        // Only check keys that have expiration set
        if (it->second.expiresAt != -1) {
            sampled++;
            
            // Check if expired
            if (it->second.isExpired()) {
                it = data.erase(it);  // Delete and advance iterator
                expired++;
            } else {
                ++it;  // Move to next key
            }
        } else {
            ++it;  // Skip keys without expiration
        }
        
        checked++;
        
        // Safety limit: don't check too many keys in one pass
        if (checked > 100) break;
    }
    
    // Recursive cleanup: if >= 25% expired, continue sampling
    if (sampled > 0 && ((float)expired / sampled) >= THRESHOLD) {
        deleteExpiredKeys();
    }
}

// ============================================================================
// EVICTION LOGIC (LRU with sampling)
// ============================================================================

// Find LRU victim by sampling random keys (Redis-style)
string Storage::findVictimLRU() {
    if (data.empty()) return "";
    
    // Seed random generator on first call
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned>(time(nullptr)));
        seeded = true;
    }
    
    int sampleSize = min(config.samplingSize, (int)data.size());
    string victim;
    int64_t oldestTime = LLONG_MAX;
    
    // Sample random keys from different positions each time
    for (int i = 0; i < sampleSize; i++) {
        // Generate random position in the map
        int randomPos = rand() % data.size();
        
        // Advance iterator to random position
        auto it = data.begin();
        advance(it, randomPos);
        
        // Check if this key is older than current victim
        if (it->second.lastAccessTime < oldestTime) {
            oldestTime = it->second.lastAccessTime;
            victim = it->first;
        }
    }
    
    return victim;
}

// Check if eviction needed and evict one key if necessary
void Storage::evictIfNeeded() {
    // Skip if no limit set or under limit
    if (config.maxKeys == 0 || data.size() < config.maxKeys) {
        return;
    }
    
    // Find and evict LRU victim
    string victim = findVictimLRU();
    if (!victim.empty()) {
        data.erase(victim);
    }
}
