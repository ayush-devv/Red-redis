// AOF Persistence Tests - Interview Showcase
// Tests append-only file functionality

#include "../include/aof.h"
#include "../include/storage.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <unistd.h>

using namespace std;

const string TEST_AOF_FILE = "test_appendonly.aof";

// Helper: Clean up test file
void cleanup() {
    remove(TEST_AOF_FILE.c_str());
}

// Test: AOF logs commands in RESP format
void test_aof_logging() {
    cleanup();
    
    {
        AOF aof(TEST_AOF_FILE, "no");  // no fsync for speed
        
        vector<string> cmd = {"SET", "key", "value"};
        aof.log(cmd);
    }  // Destructor flushes
    
    // Verify file exists and has content
    ifstream file(TEST_AOF_FILE);
    assert(file.good());
    
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    assert(content.find("SET") != string::npos);
    assert(content.find("key") != string::npos);
    assert(content.find("value") != string::npos);
    
    cleanup();
    cout << "✓ AOF logs commands in RESP format" << endl;
}

// Test: AOF replay restores data
void test_aof_replay() {
    cleanup();
    
    // Create AOF with data
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.log({"SET", "key1", "value1"});
        aof.log({"SET", "key2", "value2"});
        aof.log({"INCR", "counter"});
    }
    
    // Replay into new storage
    Storage storage;
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.replay(storage);
    }
    
    // Verify data restored
    assert(storage.get("key1").value() == "value1");
    assert(storage.get("key2").value() == "value2");
    assert(storage.get("counter").value() == "1");
    
    cleanup();
    cout << "✓ AOF replay restores data correctly" << endl;
}

// Test: AOF replay with DEL command
void test_aof_replay_with_delete() {
    cleanup();
    
    // Create AOF with SET then DEL
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.log({"SET", "temp", "value"});
        aof.log({"DEL", "temp"});
    }
    
    // Replay
    Storage storage;
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.replay(storage);
    }
    
    // Key should not exist
    assert(!storage.get("temp").has_value());
    
    cleanup();
    cout << "✓ AOF replay handles DEL correctly" << endl;
}

// Test: AOF sync modes create files
void test_aof_sync_modes() {
    cleanup();
    
    // Test "always" mode
    {
        AOF aof(TEST_AOF_FILE, "always");
        aof.log({"SET", "key", "val"});
    }
    assert(access(TEST_AOF_FILE.c_str(), F_OK) == 0);
    
    cleanup();
    
    // Test "everysec" mode
    {
        AOF aof(TEST_AOF_FILE, "everysec");
        aof.log({"SET", "key", "val"});
    }
    assert(access(TEST_AOF_FILE.c_str(), F_OK) == 0);
    
    cleanup();
    cout << "✓ AOF sync modes (always/everysec/no) work" << endl;
}

// Test: Multiple operations persist correctly
void test_aof_multiple_operations() {
    cleanup();
    
    // Write multiple operations
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.log({"SET", "k1", "v1"});
        aof.log({"INCR", "counter"});
        aof.log({"INCR", "counter"});
        aof.log({"SET", "k2", "v2"});
        aof.log({"EXPIRE", "k1", "100"});
    }
    
    // Replay
    Storage storage;
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.replay(storage);
    }
    
    // Verify all operations applied
    assert(storage.get("k1").value() == "v1");
    assert(storage.get("k2").value() == "v2");
    assert(storage.get("counter").value() == "2");
    assert(storage.getTTL("k1") > 0);  // Has expiration
    
    cleanup();
    cout << "✓ Multiple AOF operations persist correctly" << endl;
}

// Test: Empty AOF file doesn't crash
void test_empty_aof() {
    cleanup();
    
    // Create empty file
    ofstream(TEST_AOF_FILE).close();
    
    // Should not crash on replay
    Storage storage;
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.replay(storage);
    }
    
    assert(storage.size() == 0);
    
    cleanup();
    cout << "✓ Empty AOF file doesn't crash on replay" << endl;
}

// Test: AOF handles special characters
void test_aof_special_chars() {
    cleanup();
    
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.log({"SET", "key", "value\r\nwith\r\nnewlines"});
    }
    
    Storage storage;
    {
        AOF aof(TEST_AOF_FILE, "no");
        aof.replay(storage);
    }
    
    assert(storage.get("key").value() == "value\r\nwith\r\nnewlines");
    
    cleanup();
    cout << "✓ AOF handles special characters (\\r\\n)" << endl;
}

int main() {
    cout << "\n=== AOF Persistence Tests ===\n" << endl;
    
    test_aof_logging();
    test_aof_replay();
    test_aof_replay_with_delete();
    test_aof_sync_modes();
    test_aof_multiple_operations();
    test_empty_aof();
    test_aof_special_chars();
    
    cout << "\n✅ All AOF tests passed!\n" << endl;
    
    return 0;
}
