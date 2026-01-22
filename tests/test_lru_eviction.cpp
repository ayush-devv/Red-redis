#include "../include/storage.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace std;

void testEvictionWithLimit() {
    cout << "\n=== Testing LRU Eviction with maxKeys=3 ===" << endl;
    
    Storage store;
    store.setMaxKeys(3);  // Set limit to 3 keys
    
    cout << "\n1. Setting 3 keys (at limit):" << endl;
    store.set("key1", "value1");
    this_thread::sleep_for(chrono::milliseconds(10));
    cout << "   key1 set (oldest)" << endl;
    
    store.set("key2", "value2");
    this_thread::sleep_for(chrono::milliseconds(10));
    cout << "   key2 set" << endl;
    
    store.set("key3", "value3");
    this_thread::sleep_for(chrono::milliseconds(10));
    cout << "   key3 set (newest)" << endl;
    
    cout << "   Current size: " << store.size() << " / " << store.getMaxKeys() << endl;
    
    // All 3 should exist
    assert(store.exists("key1"));
    assert(store.exists("key2"));
    assert(store.exists("key3"));
    cout << "   ✓ All 3 keys confirmed present" << endl;
    
    // Access key1 to make it more recent (key2 becomes oldest)
    cout << "\n2. Accessing key1 to refresh its LRU time:" << endl;
    auto val = store.get("key1");
    assert(val.has_value());
    cout << "   key1 accessed (now most recent)" << endl;
    cout << "   LRU order: key2 (oldest), key3, key1 (newest)" << endl;
    
    // Add key4 - this should evict key2 (oldest unaccessed)
    cout << "\n3. Adding key4 (should evict key2 - the LRU victim):" << endl;
    store.set("key4", "value4");
    cout << "   key4 set" << endl;
    cout << "   Current size: " << store.size() << " / " << store.getMaxKeys() << endl;
    
    // Check results
    cout << "\n4. Checking which keys survived:" << endl;
    
    bool key1_exists = store.exists("key1");
    bool key2_exists = store.exists("key2");
    bool key3_exists = store.exists("key3");
    bool key4_exists = store.exists("key4");
    
    cout << "   key1: " << (key1_exists ? "EXISTS ✓" : "EVICTED ✗") << endl;
    cout << "   key2: " << (key2_exists ? "EXISTS ✗ (should be evicted)" : "EVICTED ✓") << endl;
    cout << "   key3: " << (key3_exists ? "EXISTS ✓" : "EVICTED ✗") << endl;
    cout << "   key4: " << (key4_exists ? "EXISTS ✓" : "EVICTED ✗") << endl;
    
    // Verify correct eviction
    assert(key1_exists && "key1 should still exist (was accessed)");
    assert(!key2_exists && "key2 should be evicted (oldest)");
    assert(key3_exists && "key3 should still exist");
    assert(key4_exists && "key4 should exist (just added)");
    
    cout << "\n✓ Eviction test passed! LRU victim correctly identified and removed." << endl;
}

void testEvictionMultiple() {
    cout << "\n=== Testing Multiple Evictions ===" << endl;
    
    Storage store;
    store.setMaxKeys(5);
    
    cout << "\n1. Filling cache with 5 keys:" << endl;
    for (int i = 1; i <= 5; i++) {
        store.set("key" + to_string(i), "value" + to_string(i));
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    cout << "   Size: " << store.size() << " / " << store.getMaxKeys() << endl;
    
    cout << "\n2. Accessing key2 and key4 (making them recent):" << endl;
    store.get("key2");
    this_thread::sleep_for(chrono::milliseconds(5));
    store.get("key4");
    cout << "   LRU order: key1 (oldest), key3, key5, key2, key4 (newest)" << endl;
    
    cout << "\n3. Adding 3 new keys (should evict key1, key3, key5):" << endl;
    store.set("keyA", "valueA");
    cout << "   Added keyA - evicted one key" << endl;
    
    store.set("keyB", "valueB");
    cout << "   Added keyB - evicted one key" << endl;
    
    store.set("keyC", "valueC");
    cout << "   Added keyC - evicted one key" << endl;
    
    cout << "\n4. Final state (should have: key2, key4, keyA, keyB, keyC):" << endl;
    cout << "   Size: " << store.size() << " / " << store.getMaxKeys() << endl;
    
    // Check survivors
    assert(store.exists("key2") && "key2 accessed, should survive");
    assert(store.exists("key4") && "key4 accessed, should survive");
    assert(store.exists("keyA") && "keyA just added, should exist");
    assert(store.exists("keyB") && "keyB just added, should exist");
    assert(store.exists("keyC") && "keyC just added, should exist");
    
    // Check evicted
    assert(!store.exists("key1") && "key1 never accessed, should be evicted");
    assert(!store.exists("key3") && "key3 never accessed, should be evicted");
    assert(!store.exists("key5") && "key5 never accessed, should be evicted");
    
    cout << "   ✓ Correct keys survived: key2, key4, keyA, keyB, keyC" << endl;
    cout << "   ✓ Correct keys evicted: key1, key3, key5" << endl;
    
    cout << "\n✓ Multiple eviction test passed!" << endl;
}

int main() {
    cout << "╔═══════════════════════════════════════╗" << endl;
    cout << "║   LRU Eviction - Comprehensive Test  ║" << endl;
    cout << "╚═══════════════════════════════════════╝" << endl;
    
    try {
        testEvictionWithLimit();
        testEvictionMultiple();
        
        cout << "\n╔═══════════════════════════════════════╗" << endl;
        cout << "║   All LRU Eviction Tests Passed! ✓   ║" << endl;
        cout << "╚═══════════════════════════════════════╝" << endl;
        
        return 0;
    } catch (const exception& e) {
        cerr << "\n✗ Test failed: " << e.what() << endl;
        return 1;
    }
}
