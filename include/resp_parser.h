
#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include "resp_value.h"
#include <string>
using namespace std;

class RespParser {
private:
    // Helper functions
    int findCRLF(const string& data, int start);
    string readLine(const string& data, int& pos);
    string readBytes(const string& data, int& pos, int n);
    int64_t parseInt(const string& str);
    
    // Type-specific parsers
    RespValue parseSimpleString(const string& data, int& pos);
    RespValue parseError(const string& data, int& pos);
    RespValue parseInteger(const string& data, int& pos);
    RespValue parseBulkString(const string& data, int& pos);
    RespValue parseArray(const string& data, int& pos);
    
public:
    // Public API
    RespValue decode(const string& data);
    // Internal decoder (public for AOF replay and pipelining)
    RespValue decodeInternal(const string& data, int& pos);
};

#endif
