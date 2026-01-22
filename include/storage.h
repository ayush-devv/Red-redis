#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <map>
#include <optional>
#include <cstdint>
using namespace std;

// Forward declare Storage for getCurrentTimeMs
class Storage;

// Configuration for eviction policy
struct Config {
    size_t maxKeys = 1000;              // Maximum number of keys (0 = unlimited)
    string evictionPolicy = "allkeys-lru";  // Eviction algorithm
    int samplingSize = 5;               // Number of keys to sample for LRU
};

// Storage value with expiration support (DiceDB-inspired)
struct StoredValue {
    string value;
    int64_t expiresAt;      // Unix timestamp in milliseconds, -1 = no expiry
    int64_t lastAccessTime; // Unix timestamp in milliseconds (for LRU)
    
    bool isExpired() const;
};

class Storage {
private:
    map<string, StoredValue> data;
    Config config;
    
    // Eviction helpers (private)
    void evictIfNeeded();          // Check and evict if over maxKeys
    string findVictimLRU();        // Sample and find LRU victim
    
public:
    // Helper: Get current time in milliseconds
    static int64_t getCurrentTimeMs();
    
    // Configuration
    void setMaxKeys(size_t maxKeys) { config.maxKeys = maxKeys; }
    size_t getMaxKeys() const { return config.maxKeys; }
    size_t size() const { return data.size(); }
    
    // Set without expiration
    void set(const string& key, const string& value);
    
    // Set with expiration (durationMs: -1 = no expiry, >0 = milliseconds from now)
    void setWithExpiry(const string& key, const string& value, int64_t durationMs);
    
    // Get value (returns nullopt if key doesn't exist or expired)
    optional<string> get(const string& key);
    
    // Check if key exists and not expired
    bool exists(const string& key);
    
    // Get TTL in seconds (-2 = doesn't exist, -1 = no expiry, N = seconds remaining)
    int64_t getTTL(const string& key);

    // Delete key (returns true if deleted, false if didn't exist)
    bool del(const string& key);

    // Set expiration on existing key (returns true if set, false if key doesn't exist)
    bool expire(const string& key, int64_t durationSec);

    // Active expiration - background cleanup (sampling approach)
    void deleteExpiredKeys();
    
    // Get all data (for AOF rewrite)
    std::map<std::string, StoredValue> getAll() const {
        return data;  // Returns copy for fork child
    }
};

#endif
