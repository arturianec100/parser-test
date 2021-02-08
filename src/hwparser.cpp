#include "hwparser.h"

#include <cctype>

HWParser::HWParser(const char *first_, const char *last_):
    first(first_), last(last_), current(first_) {}

ParseResult HWParser::parse()
{
    ParseResult result;

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

bool HWParser::isSpace() const
{
    return std::isspace(static_cast<unsigned char>(*current))
            || std::iscntrl(static_cast<unsigned char>(*current));
}

bool HWParser::shouldSkip()
{
    return ctx.isOneLineComment || ctx.isMultiLineComment
            || (ctx.isOneLineComment = (peek(2) == "//"))
            || (ctx.isMultiLineComment = (peek(2) == "/*"))
            || isSpace();
}

void HWParser::skip()
{
    //inside shouldSkip() ctx.is... are updated
    while (shouldSkip()) {

        while(isSpace()) {
            ++current;
        }

        if (ctx.isMultiLineComment) {
            while (peek(2) != "*/") {
                ++current;
            }
            current += 2;//don't point to "*/"
            ctx.isMultiLineComment = false;
        }

        if (ctx.isOneLineComment) {
            while ((*current) != '\n') {
                ++current;
            }
            ++current;//don't point to '\n'
            ctx.isOneLineComment = false;
        }
    }
}
