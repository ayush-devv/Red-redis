#ifndef RESP_ENCODER_H
#define RESP_ENCODER_H

#include <string>
#include <vector>
#include <cstdint>
using namespace std;

class RESPEncoder {
public:
    static string encodeSimpleString(const string& s);
    static string encodeError(const string& s);
    static string encodeBulkString(const string& s);
    static string encodeNull();
    static string encodeInteger(int64_t n);
    static string encodeArray(const vector<string>& arr);
};

#endif
