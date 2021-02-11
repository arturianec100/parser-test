#include "hwparser.h"

#include "macro.h"

#include <cctype>
#include <sstream>
#include <algorithm>

HWParser::HWParser(iter_type first_, iter_type last_):
    first(first_), last(last_), current(first_) {}

ParseResult HWParser::parse()
{
    ParseResult result;
    stringstream out(result.output);
    ctx = {};
    ctx.resPtr = &result;
    ctx.outPtr = &out;

    while (ctx.shouldContinue) {
        if (isEnd()) {
            break;//loop
        }
        skip();
        switch (ctx.stage) {
        case Context::NoTable: {
            if (readLeftAssignment()) {
                ctx.stage = Context::Type;
            } else {
                skipTo(';');
                step(true);
            }
            break;//switch
        }
        case Context::Type: {
            if (!readType()) {
                ctx.shouldContinue = false;
            } else {
                ctx.stage = Context::Identifier;
            }
            break;//switch
        }
        case Context::Identifier: {
            if (!readIdentifier()) {
                ctx.shouldContinue = false;
            } else {
                ctx.stage = Context::Sizing;
            }
            break;//switch
        }
        case Context::Sizing: {
            //optional, but if started must be correct
            if (!readSizing()) {
                ctx.shouldContinue = false;
            } else {
                ctx.stage = Context::Assignment;
            }
            break;//switch
        }
        case Context::Assignment: {
            if (!readAssignment()) {
                ctx.shouldContinue = false;
            } else {
                ctx.stage = Context::Table;
            }
            break;//switch
        }
        case Context::Table: {
            if (!readTable()) {
                ctx.shouldContinue = false;
            } else {
                result.ok = true;
                ctx.stage = Context::Done;
            }
            break;//switch
        }
        case Context::Done:
        default:
            ctx.shouldContinue = false;
        }
    }
    return result;
}

bool HWParser::readLeftAssignment()
{
    auto &modifiers = allowedKeyWordsModifiers;
    while (std::find(modifiers.begin(), modifiers.end(),
                     token()) != modifiers.end()) {
        moveBy(token().size());
        skip();
    }
    return true;
}

bool HWParser::readType()
{
    static const std::string_view charTypeStr = "char";
    if ((isSpecialState()) || (peek(charTypeStr.size()) != charTypeStr)) {
        (*ctx.outPtr) << "Expected \"char\" type"
               " in beginning of expression, got: " << token() << '\n';
    }
    ctx.resPtr->tableBeginIdx = pos();
    moveBy(charTypeStr.size());
    skip();
    if ((*current) != '*') {
        return false;
    }
    step();
    skip();
    //optional char* or char** or char***
    TIMES(2) {
        if (token() == "*") {
            step();
            skip();
        }
    }
    return true;
}

bool HWParser::readIdentifier()
{
    std::string_view tokenStr = token();
    if (!isIdentifier(tokenStr)) {
        (*ctx.outPtr) << "Expected identifier after type, got: " << tokenStr << '\n';
        return false;
    }
    moveBy(tokenStr.size());
    return true;
}

bool HWParser::readSizing()
{
    TIMES(2) {
        if ((*current) == '[') {
            step();
            skip();
            std::string_view tokenStr = token();
            if (!isInteger(tokenStr)) {
                (*ctx.outPtr) << "Expected integer after '[' in sizing, got: " << tokenStr << '\n';
                return false;
            }
            step();
            skip();
            if ((*current) != ']') {
                (*ctx.outPtr) << "Expected ']' after integer in sizing, got: " << tokenStr << '\n';
                return false;
            }
            step();
        }
    }
    return true;
}

bool HWParser::readAssignment()
{
    if ((*current) != '=') {
        (*ctx.outPtr) << "Expected '=' after sizing or identifier, got: " << token() << '\n';
        return false;
    }
    step();
    skip();
    return true;
}

bool HWParser::readTable()
{
    StringTable &table = ctx.resPtr->table;
    StringRow *currentRow = nullptr;
    //begin of array or arrays
    if ((*current) != '{') {
        (*ctx.outPtr) << "Expected '{' after identifier or sizing, got: " << token() << '\n';
        return false;
    }
    step();
    skip();
    if ((*current) != '{') {
        (*ctx.outPtr) << "Expected '{' inside array, got: " << token() << '\n';
        return false;
    }
    //next array or strings
    while ((*current) == '{') {
        table.append(StringRow());
        currentRow = &table.back();

        step();
        skip();
        if ((*current) != '"') {
            (*ctx.outPtr) << "Expected '\"' inside nested array, got: " << token() << '\n';
            return false;
        }
        QString tableStr;
        //next string
        while ((*current) == '"') {
            ctx.isDoubleQuotes = true;
            tableStr = "";
            if (!readString(tableStr)) {
                skipToEndOfQuotes();
            } else {
                ctx.isDoubleQuotes = false;
            }
            currentRow->append(tableStr);
            skip();
            if ((*current) != ',') {
                break;
            }
            step();
            skip();
        }//after all string of row
        if ((*current) != '}') {
            (*ctx.outPtr) << "Expected '}' after strings of nested array, got: " << token() << '\n';
            return false;
        }
        step();
        skip();
        if ((*current) == ',') {
            step();
            skip();
        }
    }//after all rows
    if ((*current) != '}') {
        (*ctx.outPtr) << "Expected '}' after nested array, got: " << token() << '\n';
        return false;
    }
    step();
    skip();
    if ((*current) != ';') {
        (*ctx.outPtr) << "Expected ';' after expression, got: " << token() << '\n';
        return false;
    }
    ctx.resPtr->tableEndIdx = pos();
    step();
    skip();
    return true;
}

bool HWParser::readString(QString &str)
{
    QTextStream stream(&str);
    step();
    while ((!isEnd()) &&
           !(((*current) == '"') && (*(current - 1) != '\\'))
           ) {
        if ((*current) == '\\') {
            step();
            continue;
        }
        if (symbolsToEscape.find(*current) != std::string::npos) {
            (*ctx.outPtr) << "Found unescaped special symbol '"
                          << (*current) << "', reading just partial string \""
                          << str.toStdString() << "\"\n";
            return false;
        }
        auto iter = escapedMapping.find(*current);
        if ((*(current - 1)) == '\\') {
            if (iter != escapedMapping.end()) {
                stream << iter->second.data();
                step();
                continue;
            }
            if (isOctal(*current)) {
                auto str = peekNOctal(3);
                stream << octal2char(str);
                current += str.size();
                continue;
            }
            if (((*current) == 'x') && isHex(*(current + 1))) {
                step();
                auto str = peekNHex(8);
                stream << hex2char(str);
                current += str.size();
                continue;
            }
            if (((*current) == 'u') && isHex(*(current + 1))) {
                step();
                auto str = peekNHex(4);
                stream << hex2char(str);
                current += str.size();
                continue;
            }
            if (((*current) == 'U') && isHex(*(current + 1))) {
                step();
                auto str = peekNHex(8);
                stream << hex2char(str);
                current += str.size();
                continue;
            }
            (*ctx.outPtr) << "Incorrect escaping syntax, found '"
                          << (*current) << "' right after \\\n";
            return false;
        }//if current char is right after \\

        stream << (*current);
        step();
    }//end of loop
    step();
    return true;
}

std::string_view HWParser::peek(std::size_t count) const
{
    return {current, count};
}

std::string_view HWParser::consume(std::size_t count)
{
    std::string_view str = peek(count);
    current += count;
    return str;
}

std::string_view HWParser::token() const
{
    iter_type iter = current;
    std::size_t length = 0;
    while ((iter < last) && isTokenChar(*iter)) {
        ++iter;
        ++length;
    }
    return {current, length};
}

std::string_view HWParser::peekNOctal(std::size_t n) const
{
    iter_type iter = current;
    std::size_t length = 0;
    while ((iter < last) && (length < n) && isOctal(*iter)) {
        ++iter;
        ++length;
    }
    return {current, length};
}

std::string_view HWParser::peekNHex(std::size_t n) const
{
    iter_type iter = current;
    std::size_t length = 0;
    while ((iter < last) && (length < n) && isHex(*iter)) {
        ++iter;
        ++length;
    }
    return {current, length};
}

void HWParser::step(bool skipQuoted)
{
    ++current;
    if (skipQuoted) {
        while ((!isEnd()) && (isOnSingleQuote() || isOnDoubleQuote())) {
            if (isOnSingleQuote() && (!isOnDoubleQuote())) {
                ctx.isSingleQuotes = !ctx.isSingleQuotes;
                step();
            }
            if (isOnDoubleQuote() && (!isOnSingleQuote())) {
                ctx.isDoubleQuotes = !ctx.isDoubleQuotes;
                step();
            }
        }
    }
}

void HWParser::moveBy(std::size_t chars)
{
    current += chars;
}

std::size_t HWParser::pos() const
{
    return static_cast<size_t>(current - first);
}

bool HWParser::isTokenChar(char c) const
{
    return (std::isalnum(static_cast<unsigned char>(c))
            || (otherTokenChars.find(c) != std::string::npos));
}

bool HWParser::isOctal(char c) const
{
    return octalChars.find(c) != std::string::npos;
}

bool HWParser::isHex(char c) const
{
    return hexChars.find(c) != std::string::npos;
}

bool HWParser::isIdentifier(std::string_view str) const
{
    return ((str[0] == '_') || std::isalpha(str[0]))
            && ((str.size() == 1) || (!std::any_of(str.begin() + 1, str.end(), [](unsigned char c) {
                return (!std::isalnum(c)) && (c != '_');
        })));
}

bool HWParser::isInteger(std::string_view str) const
{
    return !std::any_of(str.begin(), str.end(), [](unsigned char c) {
        return !std::isdigit(c);
    });
}

bool HWParser::isSpecialState() const
{
    return ctx.isOneLineComment || ctx.isMultiLineComment
            || ctx.isSingleQuotes || ctx.isDoubleQuotes;
}

bool HWParser::isSpace() const
{
    return std::isspace(static_cast<unsigned char>(*current))
            || std::iscntrl(static_cast<unsigned char>(*current));
}

bool HWParser::isEnd()
{
    return current >= last ? !(ctx.shouldContinue = false) : false;
}

bool HWParser::isOnSingleQuote() const
{
    return ((*current) == '\'') && ((*(current - 1)) != '\\');
}

bool HWParser::isOnDoubleQuote() const
{
    return ((*current) == '"') && ((*(current - 1)) != '\\');
}

bool HWParser::shouldSkip()
{
    return (!isEnd()) && (ctx.isOneLineComment || ctx.isMultiLineComment
            || (ctx.isOneLineComment = (peek(2) == "//"))
            || (ctx.isMultiLineComment = (peek(2) == "/*"))
            || isSpace());
}

void HWParser::skip()
{
    //inside shouldSkip() ctx.is...Comment are updated
    while (shouldSkip()) {

        while(isSpace() && (!isEnd())) {
            step();
        }

        if (ctx.isMultiLineComment) {
            leave_multiline_comment:
            while ((peek(2) != "*/") && (!isEnd())) {
                step();
            }
            if (!isEnd()) {
                current += 2;//don't point to "*/"
            }
            ctx.isMultiLineComment = false;
        }

        if (ctx.isOneLineComment) {
            while (((*current) != '\n') && (!isEnd())) {
                step();
                if (((*current) == '*') && ((*(current - 1)) == '/')) {
                    ctx.isMultiLineComment = true;
                    goto leave_multiline_comment;
                }
            }
            if (!isEnd()) {
                step();//don't point to '\n'
            }
            ctx.isOneLineComment = false;
        }
    }
}

void HWParser::skipTo(char c)
{
    //while (a && (b || c))
    while ((!isEnd()) && (
               ((*current) != c)
                || (ctx.isSingleQuotes
                    || ctx.isDoubleQuotes)
               )
           ) {
        if (ctx.isSingleQuotes
                || ctx.isDoubleQuotes) {
            skipToEndOfQuotes();
        }
        skip();
        step();
    }
}

void HWParser::skipToEndOfQuotes()
{
    if (ctx.isSingleQuotes) {
        while ((!isEnd()) && ctx.isSingleQuotes) {
            step();
        }
    }
    if (ctx.isDoubleQuotes) {
        while ((!isEnd()) && ctx.isDoubleQuotes) {
            step();
        }
    }
}

char HWParser::octal2char(std::string_view str) const
{
    size_t pos = 0;
    return static_cast<char>(std::stoi(string(str), &pos, 8));
}

char HWParser::hex2char(std::string_view str) const
{
    size_t pos = 0;
    return static_cast<char>(std::stoi(string(str), &pos, 16));
}
