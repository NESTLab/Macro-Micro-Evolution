#pragma once

#include "operators.h"

struct Value {
    VTYPE val;
    bool isSet;

    Value();
    Value(VTYPE val);
    Value(const Value& val);

    Value& operator=(double rs);
    Value& operator=(const Value& rs);
};

extern const Value NOVALUE;