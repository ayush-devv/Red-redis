#include "../include/storage.h"
#include "../include/command_handler.h"
#include "../include/resp_parser.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

// Helper to create RESP command
RespValue makeCommand(const vector<string>& args) {
    RespValue cmd;
    cmd.type = RespType::Array;
    for (const auto& arg : args) {
        RespValue val;
        val.type = RespType::BulkString;
        val.str_value = arg;
        cmd.arr_value.push_back(val);
    }
    return cmd;
}

void testBasicSetGet() {
    cout << "[TEST 1] Basic SET/GET without expiration... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue setCmd = makeCommand({"SET", "mykey", "hello"});
    string setResult = handler.handleCommand(setCmd);
    assert(setResult == "+OK\r\n");
    
    RespValue getCmd = makeCommand({"GET", "mykey"});
    string getResult = handler.handleCommand(getCmd);
    assert(getResult == "$5\r\nhello\r\n");
    
    cout << "PASSED" << endl;
}

void testSetWithEX() {
    cout << "[TEST 2] SET with EX (expiration in seconds)... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue setCmd = makeCommand({"SET", "expkey", "value", "EX", "2"});
    string setResult = handler.handleCommand(setCmd);
    assert(setResult == "+OK\r\n");
    
    // Get immediately - should exist
    RespValue getCmd = makeCommand({"GET", "expkey"});
    string getResult = handler.handleCommand(getCmd);
    assert(getResult == "$5\r\nvalue\r\n");
    
    // Check TTL
    RespValue ttlCmd = makeCommand({"TTL", "expkey"});
    string ttlResult = handler.handleCommand(ttlCmd);
    assert(ttlResult[0] == ':');  // Integer response
    
    // Wait 3 seconds
    this_thread::sleep_for(chrono::seconds(3));
    
    // Get after expiration - should return nil
    getResult = handler.handleCommand(getCmd);
    assert(getResult == "$-1\r\n");  // nil
    
    cout << "PASSED" << endl;
}

void testSetWithPX() {
    cout << "[TEST 3] SET with PX (expiration in milliseconds)... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue setCmd = makeCommand({"SET", "pxkey", "value", "PX", "1000"});
    string setResult = handler.handleCommand(setCmd);
    assert(setResult == "+OK\r\n");
    
    // Get immediately - should exist
    RespValue getCmd = makeCommand({"GET", "pxkey"});
    string getResult = handler.handleCommand(getCmd);
    assert(getResult == "$5\r\nvalue\r\n");
    
    // Wait 1.5 seconds
    this_thread::sleep_for(chrono::milliseconds(1500));
    
    // Get after expiration - should return nil
    getResult = handler.handleCommand(getCmd);
    assert(getResult == "$-1\r\n");
    
    cout << "PASSED" << endl;
}

void testTTLNoExpiration() {
    cout << "[TEST 4] TTL for key without expiration... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue setCmd = makeCommand({"SET", "noexpiry", "forever"});
    handler.handleCommand(setCmd);
    
    RespValue ttlCmd = makeCommand({"TTL", "noexpiry"});
    string ttlResult = handler.handleCommand(ttlCmd);
    assert(ttlResult == ":-1\r\n");  // -1 = no expiration
    
    cout << "PASSED" << endl;
}

void testTTLNonExistent() {
    cout << "[TEST 5] TTL for non-existent key... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue ttlCmd = makeCommand({"TTL", "nonexistent"});
    string ttlResult = handler.handleCommand(ttlCmd);
    assert(ttlResult == ":-2\r\n");  // -2 = doesn't exist
    
    cout << "PASSED" << endl;
}

void testInvalidEXValue() {
    cout << "[TEST 6] SET with invalid EX value... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue setCmd = makeCommand({"SET", "key", "value", "EX", "hello"});
    string setResult = handler.handleCommand(setCmd);
    assert(setResult.find("ERR value is not an integer") != string::npos);
    
    cout << "PASSED" << endl;
}

void testMissingEXValue() {
    cout << "[TEST 7] SET with missing EX value... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue setCmd = makeCommand({"SET", "key", "value", "EX"});
    string setResult = handler.handleCommand(setCmd);
    assert(setResult.find("ERR syntax error") != string::npos);
    
    cout << "PASSED" << endl;
}

void testUnknownOption() {
    cout << "[TEST 8] SET with unknown option... ";
    Storage storage;
    CommandHandler handler(storage);
    
    RespValue setCmd = makeCommand({"SET", "key", "value", "UNKNOWN"});
    string setResult = handler.handleCommand(setCmd);
    assert(setResult.find("ERR syntax error") != string::npos);
    
    cout << "PASSED" << endl;
}

void testStorageDirectly() {
    cout << "[TEST 9] Storage layer with expiration... ";
    Storage storage;
    
    // Set without expiration
    storage.set("key1", "value1");
    assert(storage.get("key1").value() == "value1");
    assert(storage.getTTL("key1") == -1);
    
    // Set with expiration
    storage.setWithExpiry("key2", "value2", 1000);  // 1 second
    assert(storage.get("key2").value() == "value2");
    assert(storage.getTTL("key2") >= 0);  // Should have TTL
    
    // Wait for expiration
    this_thread::sleep_for(chrono::milliseconds(1100));
    assert(!storage.get("key2").has_value());  // Should be expired
    assert(storage.getTTL("key2") == -2);  // Should return -2
    
    cout << "PASSED" << endl;
}

void testCommandTable() {
    cout << "[TEST 10] Command table routing... ";
    Storage storage;
    CommandHandler handler(storage);
    
    // Test PING
    RespValue pingCmd = makeCommand({"PING"});
    assert(handler.handleCommand(pingCmd) == "+PONG\r\n");
    
    // Test unknown command
    RespValue unknownCmd = makeCommand({"UNKNOWN"});
    string result = handler.handleCommand(unknownCmd);
    assert(result.find("ERR unknown command") != string::npos);
    
    // Test arity validation
    RespValue getCmd = makeCommand({"GET"});  // Missing key
    result = handler.handleCommand(getCmd);
    assert(result.find("wrong number of arguments") != string::npos);
    
    cout << "PASSED" << endl;
}

int main() {
    cout << "========================================" << endl;
    cout << "Testing Redis Clone with Expiration" << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    try {
        testBasicSetGet();
        testSetWithEX();
        testSetWithPX();
        testTTLNoExpiration();
        testTTLNonExistent();
        testInvalidEXValue();
        testMissingEXValue();
        testUnknownOption();
        testStorageDirectly();
        testCommandTable();
        
        cout << endl;
        cout << "========================================" << endl;
        cout << "✓ All 10 tests passed!" << endl;
        cout << "========================================" << endl;
        return 0;
        
    } catch (const exception& e) {
        cout << "FAILED: " << e.what() << endl;
        return 1;
    }
}
