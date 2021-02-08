#ifndef SPIRIT_PARSER_HPP
#define SPIRIT_PARSER_HPP

#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>

#include <QtCore>

#include "parseresult.h"

namespace spirit_parser {
    namespace x3 = boost::spirit::x3;
    namespace ascii = boost::spirit::x3::ascii;
    
    using x3::phrase_parse;
    using x3::_attr;
    using x3::_where;
    using x3::rule;
    //using x3::eps;
    using x3::lit;
    using x3::lexeme;
    using x3::raw;
    using x3::alpha;
    using x3::alnum;
    using x3::uint_;
    using x3::char_;
    using ascii::space;

    //grammars
    rule<class one_line_comment> const one_line_comment = "one_line_comment";
    auto const one_line_comment_def = lit("//") >> *(!char_('\n')) >> '\n';

    rule<class multi_line_comment> const multi_line_comment = "multi_line_comment";
    auto const multi_line_comment_def = lit("/*") >> *(!lit("*/")) >> lit("*/");

    rule<class skip> const skip = "skip";
    auto const skip_def = +(space | one_line_comment | multi_line_comment);

    rule<class char_type1> const char_type1 = "char_type1";
    auto const char_type1_def = lit("char") >> '*';

    rule<class char_type2> const char_type2 = "char_type2";
    auto const char_type2_def = lit("char") >> '*' >> '*';

    rule<class char_type3> const char_type3 = "char_type3";
    auto const char_type3_def = lit("char") >> '*' >> '*' >> '*';

    rule<class identifier> const identifier = "identifier";
    auto const identifier_def = lexeme[(alpha | '_') >> *(alnum | '_')];

    rule<class array_sizing> const array_sizing = "array_sizing";
    auto const array_sizing_def = char_('[') >> (-uint_) >> char_(']');

    rule<class variable_definition> const variable_definition = "variable_definition";
    auto const variable_definition_def = (char_type1 >> identifier >> array_sizing >> array_sizing)
            | (char_type2 >> identifier >> array_sizing)
            | (char_type3 >> identifier);
    //TODO: respect excape sequences
    rule<class quoted_string, std::vector<char>> const quoted_string = "quoted_string";
    auto const quoted_string_def = lexeme['"' >> +(char_ - '"') >> '"'];

    BOOST_SPIRIT_DEFINE(one_line_comment, multi_line_comment, skip,
                        char_type1, char_type2, char_type3, identifier, array_sizing,
                        variable_definition, quoted_string);

    template <typename Iterator>
    ParseResult parse_source_with_table(Iterator first, Iterator last) {
        ParseResult result;
        StringTable& table = result.table;
        QTextStream output(&result.output);
        Iterator begin_pos = first;
        
        QLinkedList<QString>* current_row = nullptr; 
        //actions
        auto print_pos = [&](auto& ctx) { output << _where(ctx); };
        auto add_cell = [&](auto& ctx) {
            std::string str(_attr(ctx).begin(), _attr(ctx).end());
            current_row->append(QString::fromStdString(str));
        };
        auto new_row = [&](auto& ctx) {
            table.append({});
            current_row = &table.back();
        };
        /*
        auto set_begin_idx = [&](auto& ctx) {
            result.tableBeginIdx = static_cast<size_t>(_attr(ctx).begin() //current pos
                                    - begin_pos);
        };
        auto set_end_idx = [&](auto& ctx) {
            result.tableEndIdx = static_cast<size_t>(_attr(ctx).begin() //current pos
                                    - begin_pos);
        };
        */
        //grammars bounded to actions
        auto const string_array = char_('{')[new_row] >> (char_('}') |
            (raw[quoted_string][add_cell] >> *(char_(',') >> raw[quoted_string][add_cell])
             >> char_('}')));
        auto const string_table = char_('{')/*[set_begin_idx]*/ >>
                                    (+string_array) >> char_('}')/*[set_end_idx]*/;
        
        result.ok = phrase_parse(first, last,
            string_table, //body parser
            skip //skip parser
        );
        return result;
    }
}

#endif // SPIRIT_PARSER_HPP
