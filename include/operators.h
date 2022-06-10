#ifndef __OPERATORS_H_
#define __OPERATORS_H_

#include <cmath>
#include <vector>
#include <stdint.h>
/**
 * 
 * "Operator Types" Defines Important Data Types And Operator Definitions
 * 
 **/


#define VTYPE double                // This is the variable type used to calculate with
#define OPERATOR_DECL VTYPE         // This is the operator declaration type - default is the variable type

namespace Operators {

    typedef VTYPE (*func)(VTYPE x, VTYPE y); // function pointer for our operator functions
    
    struct Operator { // Operator contains the function pointer and the number of operands
        func function;
        int8_t arity;
    };

    typedef std::vector<VTYPE> Variables; // Variables is a list of input variables for the equation function

/*  EqPoints structure
     result <- F(x,y,z)

     Points::points: [
        [x, y, z], [x, y, z]...
     ]
     Points::results: [
         result, result...
     ]
*/

    struct EqPoints { // EqPoints contains the list of pre-determined points and the actual calculated result for each point
        int numVars; // number of variables the equation has (number of variables per point)
        std::vector<Variables> points; // the list of points
        Variables results; // the list of caluclated results for the given points
    };

    typedef std::vector< Operator > FunctionList;


    OPERATOR_DECL Inverse(VTYPE x, VTYPE y);
    OPERATOR_DECL Negative(VTYPE x, VTYPE y);
    OPERATOR_DECL Add(VTYPE x, VTYPE y);
    OPERATOR_DECL Subtract(VTYPE x, VTYPE y);
    OPERATOR_DECL Multiply(VTYPE x, VTYPE y);
    OPERATOR_DECL Divide(VTYPE x, VTYPE y);
    OPERATOR_DECL Power(VTYPE x, VTYPE y);
    OPERATOR_DECL Abs(VTYPE x, VTYPE y);
    OPERATOR_DECL Sin(VTYPE x, VTYPE y);
    OPERATOR_DECL Cos(VTYPE x, VTYPE y);
    OPERATOR_DECL Tan(VTYPE x, VTYPE y);

}

#endif // __OPERATORS_H_