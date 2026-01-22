#include "../include/storage.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <map>

using namespace std;

void testRandomSampling() {
    cout << "\n=== Testing Random Sampling Distribution ===" << endl;
    
    Storage store;
    store.setMaxKeys(10);
    
    cout << "\n1. Creating 20 keys (will keep only 10):" << endl;
    
    // Create 20 keys with increasing timestamps
    for (int i = 0; i < 20; i++) {
        store.set("key" + to_string(i), "value" + to_string(i));
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    
    cout << "   Set 20 keys, oldest 10 should be evicted" << endl;
    cout << "   Current size: " << store.size() << " / " << store.getMaxKeys() << endl;
    
    // Check which keys survived
    cout << "\n2. Checking survivors:" << endl;
    int oldKeysCount = 0;
    int newKeysCount = 0;
    
    for (int i = 0; i < 10; i++) {
        if (store.exists("key" + to_string(i))) {
            oldKeysCount++;
        }
    }
    
    for (int i = 10; i < 20; i++) {
        if (store.exists("key" + to_string(i))) {
            newKeysCount++;
        }
    }
    
    cout << "   Old keys (0-9) surviving: " << oldKeysCount << " (should be ~0-2)" << endl;
    cout << "   New keys (10-19) surviving: " << newKeysCount << " (should be ~8-10)" << endl;
    
    // With random sampling, newer keys should mostly survive
    assert(newKeysCount >= 7 && "Most new keys should survive");
    
    cout << "\n✓ Random sampling working - newer keys mostly survived!" << endl;
}

void testSamplingFairness() {
    cout << "\n=== Testing Sampling Fairness ===" << endl;
    
    Storage store;
    store.setMaxKeys(5);
    
    cout << "\n1. Running multiple eviction cycles:" << endl;
    
    map<string, int> evictionCount;
    
    // Run multiple cycles
    for (int cycle = 0; cycle < 10; cycle++) {
        // Fill with keys
        for (int i = 0; i < 10; i++) {
            string key = "key" + to_string(i);
            store.set(key, "value");
            this_thread::sleep_for(chrono::milliseconds(2));
        }
        
        // Check which keys survived (others were evicted)
        for (int i = 0; i < 10; i++) {
            string key = "key" + to_string(i);
            if (!store.exists(key)) {
                evictionCount[key]++;
            }
        }
    }
    
    cout << "\n2. Eviction histogram (10 cycles):" << endl;
    for (const auto& [key, count] : evictionCount) {
        cout << "   " << key << ": evicted " << count << " times";
        cout << " [";
        for (int i = 0; i < count; i++) cout << "█";
        cout << "]" << endl;
    }
    
    cout << "\n   With random sampling, older keys get evicted more often" << endl;
    cout << "   (But not always - that's the randomness!)" << endl;
    
    cout << "\n✓ Sampling fairness test complete!" << endl;
}

void testLRUAccuracy() {
    cout << "\n=== Testing LRU Accuracy with Random Sampling ===" << endl;
    
    Storage store;
    store.setMaxKeys(3);
    
    cout << "\n1. Setting 3 keys:" << endl;
    store.set("key1", "val1");
    this_thread::sleep_for(chrono::milliseconds(10));
    store.set("key2", "val2");
    this_thread::sleep_for(chrono::milliseconds(10));
    store.set("key3", "val3");
    this_thread::sleep_for(chrono::milliseconds(10));
    cout << "   All 3 keys set (key1 is oldest)" << endl;
    
    cout << "\n2. Accessing key1 and key2 (making them recent):" << endl;
    store.get("key1");
    this_thread::sleep_for(chrono::milliseconds(5));
    store.get("key2");
    cout << "   LRU order: key3 (oldest), key1, key2 (newest)" << endl;
    
    cout << "\n3. Adding key4 (should likely evict key3):" << endl;
    store.set("key4", "val4");
    
    bool key1_exists = store.exists("key1");
    bool key2_exists = store.exists("key2");
    bool key3_exists = store.exists("key3");
    bool key4_exists = store.exists("key4");
    
    cout << "   key1 (accessed): " << (key1_exists ? "EXISTS ✓" : "EVICTED") << endl;
    cout << "   key2 (accessed): " << (key2_exists ? "EXISTS ✓" : "EVICTED") << endl;
    cout << "   key3 (NOT accessed): " << (key3_exists ? "EXISTS (lucky!)" : "EVICTED ✓") << endl;
    cout << "   key4 (new): " << (key4_exists ? "EXISTS ✓" : "EVICTED ✗") << endl;
    
    // key4 should always exist (just added)
    assert(key4_exists && "Newly added key should exist");
    
    // With random sampling, key3 MIGHT survive (but unlikely)
    // Accessed keys (key1, key2) have better chance of surviving
    int accessedSurvived = (key1_exists ? 1 : 0) + (key2_exists ? 1 : 0);
    
    cout << "\n   Accessed keys survived: " << accessedSurvived << "/2" << endl;
    cout << "   Note: Random sampling means results vary!" << endl;
    cout << "   (Redis achieves ~95% accuracy with 5 samples)" << endl;
    
    cout << "\n✓ LRU accuracy test complete!" << endl;
}

int main() {
    cout << "╔═══════════════════════════════════════╗" << endl;
    cout << "║   LRU Random Sampling - Test Suite   ║" << endl;
    cout << "╚═══════════════════════════════════════╝" << endl;
    
    try {
        testRandomSampling();
        testSamplingFairness();
        testLRUAccuracy();
        
        cout << "\n╔═══════════════════════════════════════╗" << endl;
        cout << "║   All Random Sampling Tests Passed!   ║" << endl;
        cout << "╚═══════════════════════════════════════╝" << endl;
        
        cout << "\nKey Insight:" << endl;
        cout << "Random sampling is probabilistic, not deterministic." << endl;
        cout << "It mimics Redis's approach: ~95% accurate, O(1) fast." << endl;
        
        return 0;
    } catch (const exception& e) {
        cerr << "\n✗ Test failed: " << e.what() << endl;
        return 1;
    }
}
