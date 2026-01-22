#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "resp_value.h"
#include "resp_encoder.h"
#include "storage.h"
#include <string>
#include <unordered_map>
#include <cstdint>
using namespace std;

// Command flags (Redis-inspired)
enum CommandFlags : uint32_t {
    CMD_WRITE    = 1 << 0,  // Modifies data
    CMD_READONLY = 1 << 1,  // Read-only
    CMD_FAST     = 1 << 2,  // Fast execution
};

class CommandHandler;

// Member function pointer type
typedef string (CommandHandler::*CommandHandlerFunc)(const RespValue&);

// Command metadata
struct CommandInfo {
    CommandHandlerFunc handler;  // Function pointer to handler
    int arity;                   // -N = at least N args, N = exactly N args
    uint32_t flags;              // Command flags
};

class CommandHandler {
private:
    Storage& storage;
    RESPEncoder encoder;
    unordered_map<string, CommandInfo> commands;  // Command table
    
    // Initialize command table
    void initCommandTable();
    
    // Helper methods
    static void toUpperCase(string& str);
    static bool parseInteger(const string& str, int64_t& out);
    
public:
    CommandHandler(Storage& store);
    string handleCommand(RespValue cmd);
    
    // Command handlers (all take same signature for function pointer)
    string handlePing(const RespValue& cmd);
    string handleSet(const RespValue& cmd);
    string handleGet(const RespValue& cmd);
    string handleTTL(const RespValue& cmd);
    string handleDel(const RespValue& cmd);
    string handleExpire(const RespValue& cmd);
    string handleIncr(const RespValue& cmd);
    string handleInfo(const RespValue& cmd);
};

#endif
