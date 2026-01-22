#include "resp_parser.h"

// Public API
RespValue RespParser::decode(const string& data) {
    int pos = 0;
    return decodeInternal(data, pos);
}

// Internal helper for recursion
RespValue RespParser::decodeInternal(const string& data, int& pos) {
    char prefix = data[pos];
    switch(prefix) {
        case '+':
            return parseSimpleString(data, pos);
        case '-':
            return parseError(data, pos);
        case ':':
            return parseInteger(data, pos);
        case '$':
            return parseBulkString(data, pos);
        case '*':
            return parseArray(data, pos);
        default:
            break;
    }
    RespValue rv;
    return rv;
}

int RespParser::findCRLF(const string& data, int start) {
    // TODO: Implement
    size_t pos = data.find("\r\n", start);
    return pos == string::npos ? -1 : static_cast<int>(pos);
}

string RespParser::readLine(const string& data, int& pos) {
    // 1. Use findCRLF to find end
    // 2. Extract substring from pos to CRLF
    // 3. Update pos to skip past \r\n
    // 4. Return the line (without \r\n)
    int end = findCRLF(data, pos);
    if (end == -1) return "";
    string line = data.substr(pos, end - pos);
    pos = end + 2; // Move past \r\n
    return line;

}

string RespParser::readBytes(const string& data, int& pos, int n) {
    // 1. Extract n bytes starting from pos
    // 2. Update pos += n
    // 3. Return the bytes
    if (pos + n > data.size()) return "";
    string bytes = data.substr(pos, n);
    pos += n;
    return bytes;
    
}

int64_t RespParser::parseInt(const string& str) {
    int64_t result=std::stoll(str);
    return result;
   
}

RespValue RespParser::parseSimpleString(const string& data, int& pos) {
    pos++;   // skip the '+'

    string str=readLine(data,pos);
    RespValue rv;
    rv.type=RespType::SimpleString;
    rv.str_value=str;
    return rv;
    
}

RespValue RespParser::parseError(const string& data, int& pos) {
    pos++; //skip "-"

    string str=readLine(data,pos);
    RespValue rv;
    rv.type=RespType::Error;
    rv.str_value=str;
    return rv;
    
}

RespValue RespParser::parseInteger(const string& data, int& pos) {
    pos++;  //skip " :"
    
    string str=readLine(data,pos);
    RespValue rv;
    rv.type=RespType::Integer;
    rv.int_value=parseInt(str);
    return rv;
    
}

RespValue RespParser::parseBulkString(const string& data, int& pos) {
    pos++;  //skip "$"

    string str_len=readLine(data,pos);
    int64_t length=parseInt(str_len);

    string str=readBytes(data,pos,length);
    pos+=2; //skip \r\n
    RespValue rv;
    rv.type=RespType::BulkString;
    rv.str_value=str;
    return rv;

    
}

RespValue RespParser::parseArray(const string& data, int& pos) {
    pos++; //skip "*"

    string str_count=readLine(data,pos);
    int64_t count=parseInt(str_count);
    RespValue rv;
    rv.type=RespType::Array;
    for(int64_t i=0;i<count;i++){
        RespValue element=decodeInternal(data, pos);  // Use internal version
        rv.arr_value.push_back(element);
    }

    return rv;
}