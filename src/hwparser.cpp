#include "hwparser.h"

#include "macro.h"

#include <cctype>
#include <algorithm>

HWParser::HWParser(iter_type first_, iter_type last_):
    first(first_), last(last_), current(first_) {}

ParseResult HWParser::parse()
{
    ParseResult result;
    QTextStream out(&result.output);
    ctx = {};
    ctx.resPtr = &result;
    ctx.outPtr = &out;

    while (ctx.shouldContinue) {
        if (isEnd()) {
            break;//loop
        }
        skip();
        switch (ctx.stage) {
        case Context::NoTable:
            if (readLeftAssignment()) {
                ctx.stage = Context::Type;
            } else {
                skipTo(';');
                step();
            }
            break;//switch
        case Context::Type:
            if (!readType()) {
                out << "Expected at least one '*' after type, got: " << token() << '\n';
                ctx.shouldContinue = false;
            } else {
                ctx.stage = Context::Identifier;
            }
            break;//switch
        case Context::Identifier:
            std::string_view tokenStr = token();
            if (!isIdentifier(tokenStr)) {
                out << "Expected identifier after type, got: " << tokenStr << '\n';
                ctx.shouldContinue = false;
                break;//switch
            }
            moveBy(tokenStr.size());
            ctx.stage = Context::Sizing;
            break;//switch
        case Context::Sizing:
            //optional, but if started must be correct
            if (!readSizing()) {
                ctx.shouldContinue = false;
            } else {
                ctx.stage = Context::Column;
            }
            break;//switch
        case Context::Table:
            if (!readTable()) {
                ctx.shouldContinue = false;
            } else {
                result.ok = true;
                ctx.stage = Context::Done;
            }
            break;//switch
        case Context::Done:
        default:
            ctx.shouldContinue = false;
        }
    }
    return result;
}

bool HWParser::readLeftAssignment()
{
    if ((!isSpecialState()) && (peek(4) == "char")) {
        ctx.resPtr->tableBeginIdx = pos();
        moveBy(4);
        return true;
    }
    return false;
}

bool HWParser::readType()
{
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
        table.append({});
        currentRow = table.back();

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
                          << str << "\"\n";
            return false;
        }
        auto iter = escapedMapping.find(*current);
        if ((*(current - 1)) == '\\') {
            if (iter != escapedMapping.end()) {
                stream << (*iter);
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

void HWParser::step()
{
    ++current;
    while ((!isEnd()) && (isOnSingleQuote() || isOnDoubleQuote())) {
        if (isOnSingleQuote() && (!isOnDoubleQuote())) {
            ctx.isSingleQuotes = !ctx.isSingleQuotes;
            ++current;
        }
        if (isOnDoubleQuote() && (!isOnSingleQuote())) {
            ctx.isDoubleQuotes = !ctx.isDoubleQuotes;
            ++current;
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
            ++current;
        }

        if (ctx.isMultiLineComment) {
            while ((peek(2) != "*/") && (!isEnd())) {
                ++current;
            }
            if (!isEnd()) {
                current += 2;//don't point to "*/"
            }
            ctx.isMultiLineComment = false;
        }

        if (ctx.isOneLineComment) {
            while (((*current) != '\n') && (!isEnd())) {
                ++current;
            }
            if (!isEnd()) {
                ++current;//don't point to '\n'
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
    return static_cast<char>(std::stoi(str, 0, 8));
}

char HWParser::hex2char(std::string_view str) const
{
    return static_cast<char>(std::stoi(str, 0, 16));
}
