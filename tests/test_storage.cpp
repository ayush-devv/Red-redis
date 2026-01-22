// Storage Layer Tests - Interview Showcase
// Tests core storage functionality and edge cases

#include "../include/storage.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace std;

// Test: Basic set and get
void test_basic_set_get() {
    Storage storage;
    storage.set("key", "value");
    
    auto result = storage.get("key");
    assert(result.has_value());
    assert(result.value() == "value");
    
    cout << "✓ Basic SET and GET work" << endl;
}

// Test: Get non-existent key returns nullopt
void test_get_missing() {
    Storage storage;
    
    auto result = storage.get("missing");
    assert(!result.has_value());
    
    cout << "✓ GET on missing key returns nullopt" << endl;
}

// Test: Delete existing key
void test_delete_key() {
    Storage storage;
    storage.set("key", "value");
    
    bool deleted = storage.del("key");
    assert(deleted == true);
    assert(!storage.get("key").has_value());
    
    cout << "✓ DEL removes existing key" << endl;
}

// Test: Delete non-existent key returns false
void test_delete_missing() {
    Storage storage;
    
    bool deleted = storage.del("missing");
    assert(deleted == false);
    
    cout << "✓ DEL on missing key returns false" << endl;
}

// Test: Expiration - key expires after TTL
void test_expiration() {
    Storage storage;
    storage.setWithExpiry("key", "value", 100);  // 100ms
    
    // Should exist immediately
    assert(storage.get("key").has_value());
    
    // Sleep past expiration
    this_thread::sleep_for(chrono::milliseconds(150));
    
    // Should be gone (lazy deletion on access)
    assert(!storage.get("key").has_value());
    
    cout << "✓ Key expires after TTL (lazy deletion)" << endl;
}

// Test: TTL returns correct values
void test_ttl_values() {
    Storage storage;
    
    // Non-existent key: -2
    assert(storage.getTTL("missing") == -2);
    
    // Key with no expiry: -1
    storage.set("persistent", "value");
    assert(storage.getTTL("persistent") == -1);
    
    // Key with expiry: positive seconds
    storage.setWithExpiry("temp", "value", 5000);  // 5 seconds
    int64_t ttl = storage.getTTL("temp");
    assert(ttl > 0 && ttl <= 5);
    
    cout << "✓ TTL returns correct values (-2, -1, positive)" << endl;
}

// Test: EXPIRE sets expiration on existing key
void test_expire_existing() {
    Storage storage;
    storage.set("key", "value");
    
    bool success = storage.expire("key", 1);
    assert(success == true);
    assert(storage.getTTL("key") > 0);
    
    cout << "✓ EXPIRE sets TTL on existing key" << endl;
}

// Test: EXPIRE on non-existent key returns false
void test_expire_missing() {
    Storage storage;
    
    bool success = storage.expire("missing", 10);
    assert(success == false);
    
    cout << "✓ EXPIRE on missing key returns false" << endl;
}

// Test: Type encoding detection
void test_type_encoding() {
    Storage storage;
    
    // Integer encoding
    storage.set("num", "12345");
    auto numVal = storage.getPtr("num");
    assert(numVal != nullptr);
    assert(storage.getEncoding(numVal->typeEncoding) == OBJ_ENCODING_INT);
    
    // Embedded string encoding (small string)
    storage.set("small", "hi");
    auto smallVal = storage.getPtr("small");
    assert(smallVal != nullptr);
    assert(storage.getEncoding(smallVal->typeEncoding) == OBJ_ENCODING_EMBSTR);
    
    // Raw encoding (large string)
    string largeStr(50, 'x');
    storage.set("large", largeStr);
    auto largeVal = storage.getPtr("large");
    assert(largeVal != nullptr);
    assert(storage.getEncoding(largeVal->typeEncoding) == OBJ_ENCODING_RAW);
    
    cout << "✓ Type encoding detection works (INT/EMBSTR/RAW)" << endl;
}

// Test: LRU eviction when maxKeys reached
void test_lru_eviction() {
    Storage storage;
    storage.setMaxKeys(3);  // Small limit
    
    // Fill to capacity
    storage.set("key1", "val1");
    this_thread::sleep_for(chrono::milliseconds(10));
    storage.set("key2", "val2");
    this_thread::sleep_for(chrono::milliseconds(10));
    storage.set("key3", "val3");
    
    // Access key1 and key2 (update LRU)
    storage.get("key1");
    storage.get("key2");
    
    // Add new key - should evict key3 (LRU)
    storage.set("key4", "val4");
    
    // key3 should be evicted (least recently used)
    assert(storage.size() == 3);
    
    cout << "✓ LRU eviction works when maxKeys exceeded" << endl;
}

// Test: Size tracking
void test_size_tracking() {
    Storage storage;
    
    assert(storage.size() == 0);
    
    storage.set("key1", "val1");
    assert(storage.size() == 1);
    
    storage.set("key2", "val2");
    assert(storage.size() == 2);
    
    storage.del("key1");
    assert(storage.size() == 1);
    
    cout << "✓ Size tracking is accurate" << endl;
}

int main() {
    cout << "\n=== Storage Layer Tests ===\n" << endl;
    
    test_basic_set_get();
    test_get_missing();
    test_delete_key();
    test_delete_missing();
    test_expiration();
    test_ttl_values();
    test_expire_existing();
    test_expire_missing();
    test_type_encoding();
    test_lru_eviction();
    test_size_tracking();
    
    cout << "\n✅ All storage tests passed!\n" << endl;
    
    return 0;
}
