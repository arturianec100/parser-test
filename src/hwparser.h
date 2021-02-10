#ifndef HWPARSER_H
#define HWPARSER_H

#include "parseresult.h"

#include <string_view>
#include <map>

using namespace std;

class HWParser
{
public:
    using iter_type = const char*;

    HWParser(iter_type first_, iter_type last_);
    //Api
    ParseResult parse();

protected:
    //Inner api
    inline bool readLeftAssignment();
    inline bool readType();
    inline bool readIdentifier();
    inline bool readSizing();
    inline bool readAssignment();
    inline bool readTable();

    inline bool readString(QString &str);

    inline string_view peek(size_t count) const;
    inline string_view consume(size_t count);
    inline string_view token() const;
    inline string_view peekNOctal(size_t n) const;
    inline string_view peekNHex(size_t n) const;
    inline void step(bool skipQuoted = false);
    inline void moveBy(size_t chars);

    inline size_t pos() const;

    inline bool isTokenChar(char c) const;
    inline bool isOctal(char c) const;
    inline bool isHex(char c) const;
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

    inline char octal2char(string_view str) const;
    inline char hex2char(string_view str) const;
    //Types
    struct Context {
        enum TableStage { NoTable = -1, Type, Identifier, Sizing,
                          Assignment, Table, Done };

        bool shouldContinue = true;
        bool isOneLineComment = false;
        bool isMultiLineComment = false;
        bool isSingleQuotes = false;
        bool isDoubleQuotes = false;
        ParseResult *resPtr = nullptr;
        stringstream *outPtr = nullptr;
        TableStage stage = NoTable;
    };
    //Static data
    inline static const vector<string> allowedKeyWordsModifiers {
        {"const"}, {"static"}, {"volatile"}
    };
    inline static const string otherTokenChars {"_"};
    inline static const string octalChars {"01234567"};
    inline static const string hexChars {"0123456789aAbBcCdDeEfF"};
    inline static const string symbolsToEscape {"\'\"\?\\\0\a\b\e\f\n\r\t\v"};
    inline static const string symbolsWithSpecialEscapeMeaning {"'\\?0abfnrtv"};
    inline static const map<char, std::string> escapedMapping {
        {'\'', "\\\'"}, {'\\', "\\\\"}, {'?', "\\?"}, {'0', "\\0"},
        {'a', "\\a"}, {'b', "\\b"}, {'e', "\\e"}, {'f', "\\f"},
        {'n', "\\n"}, {'r', "\\r"}, {'t', "\\t"}, {'v', "\\v"}
    };
    //Data
    iter_type first;
    iter_type last;
    iter_type current;
    Context ctx;
};

#endif // HWPARSER_H
