#ifndef HWPARSER_H
#define HWPARSER_H

#include "parseresult.h"

#include <string_view>

class HWParser
{
public:
    using namespace std;

    HWParser(const char* first_, const char* last_);

    ParseResult parse();

protected:
    //Api
    string_view peek(size_t count);
    string_view consume(size_t count);

    bool isSpace() const;

    bool shouldSkip();
    void skip();
    //Data
    const char* first;
    const char* last;
    char* current;
    Context ctx;
    //Types
    struct Context {
        bool isOneLineComment = false;
        bool isMultiLineComment = false;
        TableStage stage = NoTable;
        enum TableStage { NoTable = -1, Type, Identifier, Sizing, Column, Row };
    };
};

#endif // HWPARSER_H
