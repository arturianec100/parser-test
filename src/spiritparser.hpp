#ifndef SPIRIT_PARSER_HPP
#define SPIRIT_PARSER_HPP

#include <boost/spirit/home/x3.hpp>

#include <QtCore>

#include "parseresult.h"

namespace spirit_parser {
    using namespace std;
    namespace x3 = boost::spirit::x3;
    namespace ascii = boost::spirit::x3::ascii;
    
    using x3::phrase_parse;
    using x3::_attr;
    //using x3::eps;
    using x3::lit;
    using x3::lexeme;
    using x3::alpha;
    using x3::alnum;
    using x3::uint_;
    using x3::char_;
    using ascii::space;
    
    template <typename Iterator>
    ParseResult parse_source_with_table(Iterator first, Iterator last) {
        ParseResult result;
        QTextStream output(&result.output);
        //actions
        auto print = [&](auto& ctx) { output << _attr(ctx); };
        //grammars
        auto one_line_comment = lit("//") >> *(char_ - '\n') >> '\n';
        auto multi_line_comment = lit("/*") >> *(!lit("*/")) >> lit("*/");
        auto skip = +(space | one_line_comment | multi_line_comment);

        auto char_type1 = lit("char") >> '*';
        auto char_type2 = lit("char") >> '*' >> '*';
        auto char_type3 = lit("char") >> '*' >> '*' >> '*';
        auto identifier = (alpha | '_') >> *(alnum | '_');
        auto array_sizing = char_('[') >> (-uint_) >> char_(']');
        auto variable_definition = (char_type1 >> identifier >> array_sizing >> array_sizing)
                | (char_type2 >> identifier >> array_sizing)
                | (char_type3 >> identifier);
        
        result.ok = phrase_parse(first, last,
            +(char_[print]), //body parser
            skip //skip parser
        );
        return result;
    }
}

#endif // SPIRIT_PARSER_HPP
