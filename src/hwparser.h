#ifndef HWPARSER_H
#define HWPARSER_H

#include "parseresult.h"

#include <string_view>

class HWParser
{
public:
    using namespace std;

    HWParser(const char* first_, const char* last_);
    //Api
    ParseResult parse();

protected:
    //Inner api
    inline string_view peek(size_t count);
    inline string_view consume(size_t count);
    inline void step();
    inline void moveBy(size_t chars);

    inline size_t pos() const;

    inline bool isSpecialState() const;

    inline bool isSpace() const;
    inline bool isEnd();

    inline bool isOnSingleQuote() const;
    inline bool isOnDoubleQuote() const;

    inline bool shouldSkip();
    inline void skip();
    inline void skipTo(char c);
    inline void skipToEndOfQuotes();
    //Data
    const char* first;
    const char* last;
    char* current;
    Context ctx;
    //Types
    struct Context {
        bool shouldContinue = true;
        bool isOneLineComment = false;
        bool isMultiLineComment = false;
        bool isSingleQuotes = false;
        bool isDoubleQuotes = false;
        ParseResult *resPtr = nullptr;
        QTextStream *outPtr = nullptr;
        TableStage stage = NoTable;
        enum TableStage { NoTable = -1, Type, Identifier, Sizing, Column, Row };
    };
};

#endif // HWPARSER_H
