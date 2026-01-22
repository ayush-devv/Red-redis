#include "resp_parser.h"
#include <cassert>
#include <iostream>
using namespace std;

void testSimpleString() {
    RespParser parser;
    string input = "+OK\r\n";
    RespValue result = parser.decode(input);
    assert(result.type == RespType::SimpleString);
    assert(result.str_value == "OK");
}

int main() {
    testSimpleString();
    cout << "All tests passed!" << endl;
    return 0;
}