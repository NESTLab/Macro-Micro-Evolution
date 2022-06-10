#pragma once

#include "forward.h"
#include "valuetype.h"
#include "nodetypes.h"

#include <string>

class StringParser {
public:
    enum ParseType {
        PARSE_VAR, PARSE_OP, PARSE_CONST, PARSE_ERROR
    };

    struct InfoParse {
        ParseType type;
        NodeTypes::FunctionName name;
        Value value;
        std::string error;

        InfoParse(ParseType ty, NodeTypes::FunctionName name, const Value& value=NOVALUE):
            type(ty), name(name), value(value), error("") {}
        InfoParse(const std::string& error):
            type(PARSE_ERROR), name(NodeTypes::NONE), value(NOVALUE), error(error) {}
    };

    static const Parameters* params;

    static std::string errorMsg(const std::string& error, const std::string& data, size_t pos);
    static InfoParse parseError(const std::string& error, const std::string& data, size_t pos);
    static void parseWhitespace(const std::string& data, size_t& pos);
    static InfoParse getParseType(const std::string& data, size_t& pos);
    static Node* parseIteration(RootNode& rt, const std::string& data, size_t& pos);
};