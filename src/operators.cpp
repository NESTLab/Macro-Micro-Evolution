#include "operators.h"

namespace Operators {

    OPERATOR_DECL Inverse(VTYPE x, VTYPE y) {
        return VTYPE(1) / x;
    }
    OPERATOR_DECL Negative(VTYPE x, VTYPE y) {
        return VTYPE(-1) * x;
    }
    OPERATOR_DECL Add(VTYPE x, VTYPE y) {
        return x + y;
    }
    OPERATOR_DECL Subtract(VTYPE x, VTYPE y) {
        return x - y;
    }
    OPERATOR_DECL Multiply(VTYPE x, VTYPE y) {
        return x * y;
    }
    OPERATOR_DECL Divide(VTYPE x, VTYPE y) {
        return (y == 0) ? 0 : x / y;
    }
    OPERATOR_DECL Power(VTYPE x, VTYPE y) {
        return std::pow(x, y);
    }
    /* Extra Operators */
    OPERATOR_DECL Abs(VTYPE x, VTYPE y) {
        return std::abs(x);
    }
    OPERATOR_DECL Sin(VTYPE x, VTYPE y) {
        return std::sin(x);
    }
    OPERATOR_DECL Cos(VTYPE x, VTYPE y) {
        return std::cos(x);
    }
    OPERATOR_DECL Tan(VTYPE x, VTYPE y) {
        return std::tan(x);
    }
}