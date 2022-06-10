
#include "stringparser.h"
#include "evorootnode.h"
#include "parameters.h"

/*
** --- String Parser ----
*/

using namespace NodeTypes;

const Parameters* StringParser::params = nullptr;

std::string StringParser::errorMsg(const std::string& error, const std::string& data, size_t pos) {
    return std::string("Error Parsing Expression - ") + error + " -> " + data + " is invalid at position " + std::to_string(pos);
}

StringParser::InfoParse StringParser::parseError(const std::string& error, const std::string& data, size_t pos) {
    return InfoParse( errorMsg(error, data, pos) );
}

void StringParser::parseWhitespace(const std::string& data, size_t& pos) {
    while(pos < data.size() &&
          (data[pos] == ' '
          || data[pos] == '\t'
          || data[pos] == '\r'
          || data[pos] == '\n'
          || data[pos] == '\xa0'))
        pos++;
}

StringParser::InfoParse StringParser::getParseType(const std::string& data, size_t& pos) {
    for(size_t i = pos; i < data.size(); ++i){ // check for operator
        char c = data[i];
        
        if( (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')) continue;
        
        size_t end = i; // save last valid point
        parseWhitespace(data, i); // pass beyond white space
        c = data[i]; // get next valid character

        if(c == '(') { // operator found
            std::string strOp = data.substr(pos, end - pos); // store operator
            auto list = FunctionNameString;
            auto nlist = params->operatorFunctions;
            FunctionName opname = FunctionName(std::find(list.begin(), list.end(), strOp) - list.begin()); // find operator name
            if(std::find(nlist.begin(), nlist.end(), opname) == nlist.end()){ // find operator in operator list
                return parseError("Invalid Operator", strOp, pos);
            }
            pos = i; // update new position
            return InfoParse(PARSE_OP, opname);
        }

        break; // not an operator
    }
    
    if((data.size()-pos) > 3 && data.substr(pos, 3) == "var"){ // check for variable
        for(size_t i = pos+3; i < data.size(); ++i){ // variable needs to have number appended
            char c = data[i];

            if( (c < '0' || c > '9') ){
                if(pos + 3 != i){ // found a variable
                    std::string number = data.substr(pos+3, i - pos - 3);
                    Value val(std::stoi(number));
                    pos = i;
                    return InfoParse(PARSE_VAR, VARIABLE, val);
                }
                break;
            }
        }
    }
    if((data.size()-pos) > 1){   // check for constant
        char c = data[pos];
        bool issym = (c == '+' || c == '-'), dot = false, found = false;
        
        for(size_t i = pos + issym; i < data.size(); ++i){
            c = data[i];
            if(c >= '0' && c <= '9'){
                found = true;
                continue;
            }
            if(c == '.'){
                if(dot) return parseError("Invalid Constant With More Than One Significand Symbol", data.substr(pos, i - pos), pos);
                dot = true;
                continue;
            }

            if(found){
                std::string number = data.substr(pos, i - pos);
                Value val(std::stod(number));
                pos = i;
                return InfoParse(PARSE_CONST, CONSTANT, val);
            }
            break;
        }
    }

    return parseError("Unexpected Symbol In Expression", data.substr(pos, std::min(size_t(8), data.size()-pos) ), pos);
}

Node* StringParser::parseIteration(RootNode& rt, const std::string& data, size_t& pos) {
    parseWhitespace(data, pos);
    InfoParse v = getParseType(data, pos);
    if(v.type == PARSE_ERROR){
        warning(v.error);
        return nullptr;
    }

    Node* newNode = rt.createNode(v.name);

    auto checkChar = [&](char in, const std::string& error) -> bool {
        if(pos >= data.size()){
            warning(errorMsg("Unexpected End", std::string(1, data[pos-1]), pos));
            newNode->freeAll();
            return false;
        }
        if(data[pos] != in){
            warning(errorMsg(error, std::string(1, data[pos]), pos));
            newNode->freeAll();
            return false;
        }
        pos++;
        return true;
    };

    switch(v.type){
        case PARSE_OP:{
            Node* child;
            OpNode* opNode = static_cast<OpNode*>(newNode);
            
            if(!checkChar('(',"Expected Operator '(' Initializer; Unexpected Symbol")) return nullptr;
            parseWhitespace(data, pos);

            // First Operand
            child = parseIteration(rt, data, pos); // get first child

            if(child == nullptr){ // failed to parse
                newNode->freeAll();
                return nullptr;
            }
            opNode->setchild(0, child);

            //Second Operand
            if(opNode->arity == 2){
                parseWhitespace(data, pos);
                if(!checkChar(',', "Expected ',' In Expression; Unexpected Symbol")) return nullptr;
                parseWhitespace(data, pos);

                child = parseIteration(rt, data, pos); // get second child

                if(child == nullptr){ // failed to parse
                    newNode->freeAll();
                    return nullptr;
                }
                opNode->setchild(1, child);
            }
            parseWhitespace(data, pos);
            if(!checkChar(')', "Expected ')' Terminator In Expression; Unexpected Symbol")) return nullptr;
            parseWhitespace(data, pos);

            return opNode;
        }
        case PARSE_CONST:
        case PARSE_VAR:{
            VarNode* varNode = static_cast<VarNode*>(newNode);
            varNode->setVal(v.value.val);
            return newNode;
        }
    }

    warning(errorMsg("A Serious Parse Failure Occurred", "<n/a>", pos));
    newNode->freeAll();
    return nullptr;
}