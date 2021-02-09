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
    inline bool readLeftAssignment();
    inline bool readType();
    inline bool readSizing();
    inline bool readTable();

    inline string_view peek(size_t count);
    inline string_view consume(size_t count);
    inline string_view token();
    inline void step();
    inline void moveBy(size_t chars);

    inline size_t pos() const;

    inline bool isTokenChar(char c) const;
    inline bool isIdentifier(string_view str) const;
    inline bool isInteger(string_view str) const;

    inline bool isSpecialState() const;

    inline bool isSpace() const;
    inline bool isEnd();

    inline bool isOnSingleQuote() const;
    inline bool isOnDoubleQuote() const;

    inline bool shouldSkip();
    inline void skip();
    inline void skipTo(char c);
    inline void skipToEndOfQuotes();
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
        enum TableStage { NoTable = -1, Type, Identifier, Sizing, Table, Done };
    };
    //Data
    const char* first;
    const char* last;
    const char* current;
    Context ctx;
};

#endif // HWPARSER_H
