#ifndef PARSER_HPP
#define PARSER_HPP

#include "parseresult.h"

#ifdef USE_SPIRIT_PARSER

#include "spiritparser.hpp"

#else

#include "hwparser.h"

#endif // USE_SPIRIT_PARSER

ParseResult parse_source(const char* text, const char* end) {
#ifdef USE_SPIRIT_PARSER
    return spirit_parser::parse_source_with_table(text, end);
#endif // USE_SPIRIT_PARSER
    HWParser parser(text, end);
    return parser.parse();
}

#endif // PARSER_HPP
