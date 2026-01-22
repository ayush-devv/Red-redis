#ifndef RESP_ENCODER_H
#define RESP_ENCODER_H

#include <string>
#include <cstdint>
using namespace std;

class RespEncoder {
public:
    string encodeSimpleString(const string& s);
    string encodeError(const string& s);
    string encodeBulkString(const string& s);
    string encodeNull();
    string encodeInteger(int64_t n);
};

#endif
