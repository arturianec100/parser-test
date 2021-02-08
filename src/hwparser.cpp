#include "hwparser.h"

#include <cctype>

HWParser::HWParser(const char *first_, const char *last_):
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
            if ((!isSpecialState()) && (peek(4) == "char")) {
                ctx.stage = Context::Type;
                moveBy(4);
                break;//switch
            }
            skipTo(';');
            ++current;
            break;
        case Context::Type:
            //
            break;
        default:
            ctx.shouldContinue = false;
        }
    }

    return result;
}

std::string_view HWParser::peek(std::size_t count)
{
    return {current, count};
}

std::string_view HWParser::consume(std::size_t count)
{
    std::string_view str = peek(count);
    current += count;
    return str;
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
