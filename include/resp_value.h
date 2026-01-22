
#ifndef RESP_VALUE_H
#define RESP_VALUE_H

#include <string>
#include <vector>
#include <cstdint>
using namespace std;

enum class RespType {
    SimpleString,  // +OK\r\n
    Error,         // -Error\r\n
    Integer,       // :1000\r\n
    BulkString,    // $5\r\nhello\r\n
    Array          // *2\r\n...
};

struct RespValue {
    RespType type;
    
    string str_value;            // For SimpleString, Error, BulkString
    int64_t int_value;           // For Integer
    vector<RespValue> arr_value; // For Array (recursive!)
};

#endif
