#ifndef __NODETYPES_H_
#define __NODETYPES_H_

#include <string>
#include <vector>

namespace NodeTypes {
    typedef const std::vector<std::string> EnumStringMap;
    
    enum FunctionName {
        NONE, RANDOM_OP, RANDOM_VAR,
        CONSTANT, VARIABLE,
        INVERSE, NEGATIVE,
        ADD, SUBTRACT,
        MULTIPLY, DIVIDE, POWER,
        ABS, SIN, COS, TAN
    };

    static EnumStringMap FunctionNameString {"none", "rand_op", "rand_var",
                                             "const", "var", "inv", "neg",
                                             "add", "sub", "mul", "div", "pow",
                                             "abs", "sin", "cos", "tan"};
}


#endif // __NODETYPES_H_