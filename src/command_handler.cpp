#include "command_handler.h"
#include <algorithm>
#include <cctype>

// Constructor - initialize command table
CommandHandler::CommandHandler(Storage& store) : storage(store) {
    initCommandTable();
}

// Initialize command table (Redis-inspired)
void CommandHandler::initCommandTable() {
    commands["PING"] = {&CommandHandler::handlePing, -1, CMD_FAST | CMD_READONLY};
    commands["SET"]  = {&CommandHandler::handleSet,  -3, CMD_WRITE};
    commands["GET"]  = {&CommandHandler::handleGet,   2, CMD_READONLY | CMD_FAST};
    commands["TTL"]  = {&CommandHandler::handleTTL,   2, CMD_READONLY | CMD_FAST};
    commands["DEL"]  = {&CommandHandler::handleDel,  -2, CMD_WRITE};
    commands["EXPIRE"] = {&CommandHandler::handleExpire, 3, CMD_WRITE};

}

// Helper: Convert string to uppercase
void CommandHandler::toUpperCase(string& str) {
    for (char& c : str) c = toupper(c);
}

// Helper: Parse integer with validation
bool CommandHandler::parseInteger(const string& str, int64_t& out) {
    try {
        size_t pos;
        out = stoll(str, &pos);
        return pos == str.length();  // Ensure entire string was parsed
    } catch (...) {
        return false;
    }
}

// Main command dispatcher with table lookup
string CommandHandler::handleCommand(RespValue cmd) {
    if (cmd.type != RespType::Array || cmd.arr_value.empty()) {
        return encoder.encodeError("ERR invalid command");
    }
    
    // Get command name (uppercase)
    string cmdName = cmd.arr_value[0].str_value;
    toUpperCase(cmdName);
    
    // Lookup command in table
    auto it = commands.find(cmdName);
    if (it == commands.end()) {
        return encoder.encodeError("ERR unknown command '" + cmdName + "'");
    }
    
    const CommandInfo& cmdInfo = it->second;
    int argc = cmd.arr_value.size();
    
    // Validate arity (Redis style: negative = at least, positive = exactly)
    if (cmdInfo.arity > 0 && argc != cmdInfo.arity) {
        return encoder.encodeError("ERR wrong number of arguments for '" + cmdName + "' command");
    }
    if (cmdInfo.arity < 0 && argc < -cmdInfo.arity) {
        return encoder.encodeError("ERR wrong number of arguments for '" + cmdName + "' command");
    }
    
    // Call handler using member function pointer
    return (this->*(cmdInfo.handler))(cmd);
}

// PING command handler
string CommandHandler::handlePing(const RespValue& cmd) {
    return encoder.encodeSimpleString("PONG");
}

// SET command handler with EX/PX support (DiceDB-inspired)
string CommandHandler::handleSet(const RespValue& cmd) {
    string key = cmd.arr_value[1].str_value;
    string val = cmd.arr_value[2].str_value;
    int64_t expiryMs = -1;  // -1 = no expiration
    
    // Parse options starting at index 3
    for (size_t i = 3; i < cmd.arr_value.size(); i++) {
        string opt = cmd.arr_value[i].str_value;
        toUpperCase(opt);
        
        if (opt == "EX") {
            // EX seconds - expiry in seconds
            if (i + 1 >= cmd.arr_value.size()) {
                return encoder.encodeError("ERR syntax error");
            }
            i++;
            int64_t seconds;
            if (!parseInteger(cmd.arr_value[i].str_value, seconds) || seconds <= 0) {
                return encoder.encodeError("ERR value is not an integer or out of range");
            }
            expiryMs = seconds * 1000;  // Convert to milliseconds
            
        } else if (opt == "PX") {
            // PX milliseconds - expiry in milliseconds
            if (i + 1 >= cmd.arr_value.size()) {
                return encoder.encodeError("ERR syntax error");
            }
            i++;
            int64_t milliseconds;
            if (!parseInteger(cmd.arr_value[i].str_value, milliseconds) || milliseconds <= 0) {
                return encoder.encodeError("ERR value is not an integer or out of range");
            }
            expiryMs = milliseconds;
            
        } else {
            return encoder.encodeError("ERR syntax error");
        }
    }
    
    // Store with expiration
    storage.setWithExpiry(key, val, expiryMs);
    return encoder.encodeSimpleString("OK");
}

// GET command handler with expiration check
string CommandHandler::handleGet(const RespValue& cmd) {
    string key = cmd.arr_value[1].str_value;
    
    // Get value (returns nullopt if expired or doesn't exist)
    optional<string> value = storage.get(key);
    
    if (value.has_value()) {
        return encoder.encodeBulkString(value.value());
    }
    return encoder.encodeNull();
}

// TTL command handler (DiceDB-inspired)
string CommandHandler::handleTTL(const RespValue& cmd) {
    string key = cmd.arr_value[1].str_value;
    
    int64_t ttl = storage.getTTL(key);
    return encoder.encodeInteger(ttl);
}

// DEL command handler 
string CommandHandler::handleDel(const RespValue& cmd) {
    int countDeleted = 0;
    
    // Loop through all keys starting from index 1
    for (size_t i = 1; i < cmd.arr_value.size(); i++) {
        string key = cmd.arr_value[i].str_value;
        if (storage.del(key)) {
            countDeleted++;
        }
    }
    
    return encoder.encodeInteger(countDeleted);
}

// EXPIRE command handler 
string CommandHandler::handleExpire(const RespValue& cmd) {
    string key = cmd.arr_value[1].str_value;
    
    // Parse seconds
    int64_t seconds;
    if (!parseInteger(cmd.arr_value[2].str_value, seconds)) {
        return encoder.encodeError("ERR value is not an integer or out of range");
    }
    
    // Try to set expiration
    bool success = storage.expire(key, seconds);
    
    return encoder.encodeInteger(success ? 1 : 0);
}
