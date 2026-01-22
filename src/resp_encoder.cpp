#include "resp_encoder.h"

string RespEncoder::encodeSimpleString(const string& s) {
    return "+" + s + "\r\n";
}

string RespEncoder::encodeError(const string& s) {
    return "-" + s + "\r\n";
}

string RespEncoder::encodeBulkString(const string& s) {
    return "$" + to_string(s.size()) + "\r\n" + s + "\r\n";
}

string RespEncoder::encodeNull() {
    return "$-1\r\n";
}

string RespEncoder::encodeInteger(int64_t n) {
    return ":" + to_string(n) + "\r\n";
}
