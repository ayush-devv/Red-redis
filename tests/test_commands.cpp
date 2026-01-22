// Command Handler Tests - Interview Showcase
// Tests core Redis commands with edge cases

#include "../include/command_handler.h"
#include "../include/storage.h"
#include "../include/resp_parser.h"
#include <iostream>
#include <cassert>
#include <string>

using namespace std;

// Helper to parse command string to RESPValue
RESPValue parseCommand(const string& cmdStr) {
    RespParser parser;
    return parser.decode(cmdStr);
}

// Test: INCR creates new key with value 1
void test_incr_new_key() {
    Storage storage;
    CommandHandler handler(storage);
    
    RESPValue cmd = parseCommand("*2\r\n$4\r\nINCR\r\n$7\r\ncounter\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result == ":1\r\n");
    assert(storage.get("counter").value() == "1");
    
    cout << "✓ INCR on new key creates value 1" << endl;
}

// Test: INCR increments existing integer
void test_incr_existing() {
    Storage storage;
    CommandHandler handler(storage);
    
    storage.set("counter", "10");
    
    RESPValue cmd = parseCommand("*2\r\n$4\r\nINCR\r\n$7\r\ncounter\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result == ":11\r\n");
    assert(storage.get("counter").value() == "11");
    
    cout << "✓ INCR increments existing integer" << endl;
}

// Test: INCR on non-integer returns error
void test_incr_non_integer() {
    Storage storage;
    CommandHandler handler(storage);
    
    storage.set("name", "Alice");
    
    RESPValue cmd = parseCommand("*2\r\n$4\r\nINCR\r\n$4\r\nname\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result.find("ERR") != string::npos);
    assert(result.find("integer") != string::npos);
    
    cout << "✓ INCR on non-integer returns error" << endl;
}

// Test: SET with EX option sets expiration
void test_set_with_expiration() {
    Storage storage;
    CommandHandler handler(storage);
    
    RESPValue cmd = parseCommand("*5\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n$2\r\nEX\r\n$1\r\n1\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result == "+OK\r\n");
    assert(storage.get("key").value() == "value");
    assert(storage.getTTL("key") > 0);
    
    cout << "✓ SET with EX sets expiration" << endl;
}

// Test: GET returns nil for non-existent key
void test_get_nonexistent() {
    Storage storage;
    CommandHandler handler(storage);
    
    RESPValue cmd = parseCommand("*2\r\n$3\r\nGET\r\n$7\r\nmissing\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result == "$-1\r\n");  // RESP nil
    
    cout << "✓ GET on non-existent key returns nil" << endl;
}

// Test: DEL removes key and returns count
void test_del_multiple() {
    Storage storage;
    CommandHandler handler(storage);
    
    storage.set("key1", "val1");
    storage.set("key2", "val2");
    
    RESPValue cmd = parseCommand("*3\r\n$3\r\nDEL\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result == ":2\r\n");
    assert(!storage.get("key1").has_value());
    
    cout << "✓ DEL removes multiple keys" << endl;
}

// Test: INFO returns keyspace statistics
void test_info_keyspace() {
    Storage storage;
    CommandHandler handler(storage);
    
    storage.set("key1", "val1");
    storage.setWithExpiry("key2", "val2", 5000);
    
    RESPValue cmd = parseCommand("*1\r\n$4\r\nINFO\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result.find("Keyspace") != string::npos);
    assert(result.find("keys=2") != string::npos);
    assert(result.find("expires=1") != string::npos);
    
    cout << "✓ INFO returns correct keyspace stats" << endl;
}

// Test: EXPIRE sets TTL on existing key
void test_expire_key() {
    Storage storage;
    CommandHandler handler(storage);
    
    storage.set("key", "value");
    
    RESPValue cmd = parseCommand("*3\r\n$6\r\nEXPIRE\r\n$3\r\nkey\r\n$2\r\n10\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result == ":1\r\n");
    assert(storage.getTTL("key") > 0);
    
    cout << "✓ EXPIRE sets TTL on existing key" << endl;
}

// Test: TTL returns -2 for non-existent key
void test_ttl_nonexistent() {
    Storage storage;
    CommandHandler handler(storage);
    
    RESPValue cmd = parseCommand("*2\r\n$3\r\nTTL\r\n$7\r\nmissing\r\n");
    string result = handler.handleCommand(cmd);
    
    assert(result == ":-2\r\n");
    
    cout << "✓ TTL returns -2 for non-existent key" << endl;
}

int main() {
    cout << "\n=== Command Handler Tests ===\n" << endl;
    
    test_incr_new_key();
    test_incr_existing();
    test_incr_non_integer();
    test_set_with_expiration();
    test_get_nonexistent();
    test_del_multiple();
    test_info_keyspace();
    test_expire_key();
    test_ttl_nonexistent();
    
    cout << "\n✅ All command tests passed!\n" << endl;
    
    return 0;
}
