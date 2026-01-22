#include "../include/resp_encoder.h"

string RESPEncoder::encodeSimpleString(const string& s) {
    return "+" + s + "\r\n";
}

string RESPEncoder::encodeError(const string& s) {
    return "-" + s + "\r\n";
}

string RESPEncoder::encodeBulkString(const string& s) {
    return "$" + to_string(s.size()) + "\r\n" + s + "\r\n";
}

string RESPEncoder::encodeNull() {
    return "$-1\r\n";
}

string RESPEncoder::encodeInteger(int64_t n) {
    return ":" + to_string(n) + "\r\n";
}

string RESPEncoder::encodeArray(const vector<string>& arr) {
    string result = "*" + to_string(arr.size()) + "\r\n";
    for (const auto& item : arr) {
        result += encodeBulkString(item);
    }
    return result;
}
