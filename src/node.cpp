#include "node.h"

using namespace NodeTypes;

const Value NOVALUE;

Value::Value(const Value& value): val(value.val), isSet(value.isSet) {}
Value::Value(VTYPE value): val(value), isSet(true) {}
Value::Value(): isSet(false) {}
Value& Value::operator=(double rs) {
    val = rs;
    isSet = true;
    return *this;
}
Value& Value::operator=(const Value& rs) {
    val = rs.val;
    isSet = rs.isSet;
    return *this;
}

NodeList& NodeList::operator+=(const NodeList& rs) {
    for(Node* n : rs.constants) constants.push_back(n);
    for(Node* n : rs.operators) operators.push_back(n);
    for(Node* n : rs.variables) variables.push_back(n);
    for(Node* n : rs.all) all.push_back(n);
    return *this;
}


/**
 *      Base Node:
**/

std::atomic<size_t> Node::debugSkippedSimplify = 0;

void Node::ConstructNode(RootNode* rootnode, FunctionName fname, Node* parent, const Children& _children, int8_t arity) {
    rootNode = rootnode;
    name = fname;
    this->parent = parent;
    this->arity = arity;
    if(rootNode->params == nullptr) throw std::runtime_error("Parameters missing"); // cannot continue because parameters were denied
    children[0] = _children[0];
    children[1] = _children[1];

}

Node::Node(): memory(nullptr), parent(nullptr), rootNode(nullptr) { // initialize empty node
}

Node::~Node() {
}

void Node::free() {
    rootNode->pool.deallocate_Node(this);
}

void Node::freeAll() {
    free();
}


float Node::rmsCalculate(const std::vector<VTYPE>& actual, const std::vector<VTYPE>& results) {
    if(actual.size() != results.size()) throw std::runtime_error("Equation points do not match the length of results!");
    VTYPE min = Parameters::Params()->minRMSClamp , max = Parameters::Params()->maxRMSClamp;
    float score = 0;
    for(size_t i=0; i<actual.size(); ++i){
        // clamp the actual data and results to user-defined values
        VTYPE sum = std::clamp(actual[i], min, max) - std::clamp(results[i], min, max);
        score += std::pow(sum, 2);
    }
    if(std::isnan(score)){
        score = INFINITY;
    } else {
        score /= float(actual.size());
        if(Parameters::Params()->useSqrtRMS)
            score = std::sqrt(score);
    }
    return score;
}

double Node::computeComplexity() {
    double cumulative = 0, complexity = 0;

    Parameters::OperandType ls = Parameters::NONE,
                            rs = Parameters::NONE;

    switch(arity){
        case 0: return 0; // no complexity
        case 2:{ // 2 operands
            Node* c = child(1);
            rs = (c->name == CONSTANT) ? Parameters::CONSTANT : Parameters::OPERATOR;
            cumulative += c->computeComplexity();
        } [[fallthrough]];
        case 1:{ // 2 or 1 operands - always calculate lhs complexity if at least 1 operand
            Node* c = child(0);
            ls = (c->name == CONSTANT) ? Parameters::CONSTANT : Parameters::OPERATOR;
            cumulative += c->computeComplexity();
            break;
        }
        default: { // having 3 or more children is invalid
            throw std::runtime_error("More than 2 children found in Node");
        }
    }

    const ComplexityList& list = rootNode->params->complexity[name];
    
    ComplexityList::const_iterator pos = std::find_if(list.begin(), list.end(),
            [&](ComplexityOp op) -> bool { return (ls == op.ls && rs == op.rs); }
        );

    complexity += cumulative + (pos == list.end() ? rootNode->params->defaultComplexity : pos->complexity);

    return complexity;
}

Node* Node::copy(RootNode* newRootNode) const {
    Node* node = newRootNode->createNode(name); // instantiate new node
    for(int i=0; i < arity; ++i){ // copy all children if any
        node->setchild(i, child(i)->copy(newRootNode));
    }
    return node;
}

Node* Node::copyMutate(RootNode* newRootNode, const Node* to, const Node* from) const {
    Node* node;
    if(to == this) { // this is the node that will be replaced
        return from->copy(newRootNode); // return a copy of the mutation branch
    }
    node = newRootNode->createNode(name); // instantiate new node

    // Perform all tasks for each node type
    if(arity > 0){ // determine if node is an OpNode or a VarNode based on its arity
        for(int i=0; i < arity; ++i){ // copy all children
            node->setchild(i, child(i)->copyMutate(newRootNode, to, from)); // recursive search for the to-node
        }
    } else {
        ((VarNode*)node)->setVal(((VarNode*) this)->value.val);
        return node;
    }
    return node;
}

// Call this function on the children nodes
void Node::setParent(Node* parent) {
    this->parent = parent;
}

// Call this function on the root node to iterate all children and update their parents
void Node::updateLinks() {
    for(int i=0;i < arity;++i) {
        if(child(i)->parent != this) {
            child(i)->setParent(this);
            warning("found a broken child link!");
        }
        child(i)->updateLinks();
    }
}

void Node::listOfNodes(NodeList& list) const {}

void Node::cout(int level) const {
    std::string tabs(level*4, ' ');
    syslog::cout << tabs << "Parent: " << (parent == nullptr ? "null" : to_hex_string(parent)) << "\n"
              << tabs << "Children: " << (int)arity << "\n";
    
    syslog::cout << tabs << "**************************\n";
    for(int i=0; i < arity; ++i){
        VarNode* vnode = (VarNode*)children[i];
        OpNode* opnode = (OpNode*)children[i];

        if(children[i]->name == CONSTANT || children[i]->name == VARIABLE) {
            vnode->cout(level+1);
        } else {
            opnode->cout(level+1);
        }
        syslog::cout << tabs << "**************************\n";
    }
}

float Node::score(const Operators::EqPoints& points, bool evo) { // note that this function is called for both micro and macro scoring
    float score;
    if(evo){
        // Use fitness evolution algorithm here - Kodi: *Woof!* *Woof!*
        NodeList nodes;
        Fitness fit(rootNode, points, nodes); // instantiate the fitness algorithm
        score = fit.run(); // run the fitness algorithm magic and get the new score back
    } else {
        std::vector<VTYPE> myResults;
        myResults.reserve(points.results.size());
        for(const Operators::Variables& vars : points.points){
            myResults.push_back(compute(vars));
        }

        score = rmsCalculate(points.results, myResults);
    }
    return score;
}


Node* Node::simplify() {
    if(arity == 0) return nullptr;
    OpNode* me = static_cast<OpNode*>(this);

    for(int i=0; i < arity; ++i) {
        OpNode* c = (OpNode*) (child(i)->simplify());
        //OpNode* c = (OpNode*) (child(i)->simplify(me, i)); // Potential New Method
        if(c == nullptr) continue;
        if(child(i)->parent == this) freechild(i); // free old memory
        setchild(i, c); // update child (auto update parent)

    }

    if(rootNode->params->denySimplifyOperator == name){
        debugSkippedSimplify++; // counter increment
        return nullptr; // skip based on user defined parameter - do not simplify
    }

    Node* r; // temp return node
    switch(name){
        case INVERSE:{
            if(child(0)->name == CONSTANT){ // inv(c) = -c
                varchild(0)->setVal(VTYPE(1) / varchild(0)->value.val);
                return child(0);
            }
            if(child(0)->name == INVERSE){ // inv(inv(a)) = a
                r = opchild(0)->child(0);
                freechild(0); // free sub-child memory
                return r;
            }
            if(child(0)->name == POWER){ // inv(pow(a,c)) = pow(a,-c)
                if(opchild(0)->child(1)->name == CONSTANT){
                    VarNode* n = opchild(0)->varchild(1);
                    n->setVal(n->value.val * VTYPE(-1)); // negate the constant
                } else {
                    r = rootNode->createNode(NEGATIVE); // negate exponent
                    r->setchild(0, opchild(0)->child(1)); // set child (auto-updates parent)
                    opchild(0)->setchild(1, r); // update exponent to negative operator (auto-updates parent)
                }
                return child(0);
            }
            if(child(0)->name == DIVIDE){ // inv(div(a,b)) = div(b,a)
                opchild(0)->swap();
                return child(0);
            }

            break;
        }
        case NEGATIVE:{
            if(child(0)->name == CONSTANT){ // neg(c) = -c
                varchild(0)->setVal(varchild(0)->value.val * VTYPE(-1));
                return child(0);
            }
            if(child(0)->name == NEGATIVE){ // neg(neg(a)) = a
                r = opchild(0)->child(0); // get inner inner node
                freechild(0); // free inner op node
                return r;
            }
            if(child(0)->name == SUBTRACT){ // neg(sub(a,b)) = sub(b,a)
                opchild(0)->swap();
                return child(0);
            }
            if(child(0)->name == ADD && opchild(0)->child(0)->name == CONSTANT){ // neg(add(c,a)) = sub(-c,a)
                opchild(0)->varchild(0)->setVal(opchild(0)->varchild(0)->value.val * VTYPE(-1));
                child(0)->changeOperator(SUBTRACT);
                return child(0);
            }

            break;
        }
        case ADD:{
            if(child(0)->name == CONSTANT && child(1)->name == CONSTANT){ // add(c,c) = c
                varchild(0)->setVal(varchild(0)->value.val + varchild(1)->value.val);
                freechild(1, true); // free second constant
                return child(0);
            }
            if(child(0)->name == NEGATIVE && child(1)->name == NEGATIVE){ // add(neg(a),neg(b)) = neg(add(a,b))
                OpNode *ls = opchild(0), *rs = opchild(1);
                setchild(0, ls->child(0)); // update me new children
                setchild(1, rs->child(0));

                rs->free(); // destroy other op because it's no longer used
                ls->setchild(0, this); // top op child goes above (auto-update parent)

                return ls;
            }
            if(child(0)->name == NEGATIVE){ // add(neg(a),b) = add(b,neg(a)),  continue
                me->swap(); // swap my children
            }
            if(child(1)->name == NEGATIVE){ // add(a,neg(b)) = sub(a,b)
                changeOperator(SUBTRACT);
                r = child(1);
                setchild(1, opchild(1)->child(0));
                r->free(); // free negative op
                return nullptr; // do not kill me!
            }
            
            if(child(0)->name == ADD && child(1)->name == ADD) { // add(add(c,o),add(c,o)) = add(c,add(o,o))
                if(opchild(0)->child(1)->name == CONSTANT) opchild(0)->swap();
                if(opchild(1)->child(1)->name == CONSTANT) opchild(1)->swap();

                if(opchild(0)->child(0)->name == CONSTANT && opchild(1)->child(0)->name == CONSTANT){
                    VarNode *ls = opchild(0)->varchild(0), *rs = opchild(1)->varchild(0);
                    ls->setVal(ls->value.val + rs->value.val);
                    // drop second old constant
                    rs->free();
                    // move leftside opnode into rightside add 
                    opchild(1)->setchild(0, opchild(0)->child(1));
                    // move summed constant up from leftside
                    freechild(0);
                    setchild(0, ls);
                }
                return nullptr;
            }

            if(child(1)->name == CONSTANT){ // add(o,c) = add(c,o)
                me->swap();
            }
            
            if(child(0)->name == CONSTANT){
                if(varchild(0)->value.val == 0){ // add(0, a) = a
                    freechild(0, true);
                    return child(1);
                }
                if(child(1)->name == ADD){ // add(c,add(c,o)) = add(c,o)
                    if(opchild(1)->child(0)->name == CONSTANT){
                        varchild(0)->setVal(varchild(0)->value.val + opchild(1)->varchild(0)->value.val);
                        r = opchild(1)->child(1);
                        opchild(1)->freechild(0); // free constant
                        freechild(1); // free opnode
                        setchild(1, r);
                    }
                    return nullptr;
                }
            }

            if(child(0)->name == VARIABLE && child(1)->name == VARIABLE && varchild(0)->value.val == varchild(1)->value.val) { // add(x,x) = mul(x,2)
                changeOperator(MULTIPLY);
                child(0)->changeOperator(CONSTANT);
                varchild(0)->setVal(2);
                return nullptr;
            }

            // add(___(x),___(x)) = mul(___(x), 2)
            if( (child(0)->name == SIN || child(0)->name == COS || child(0)->name == TAN ||
                child(0)->name == ABS || child(0)->name == INVERSE || child(0)->name == NEGATIVE) &&
                child(1)->name == child(0)->name &&
                opchild(0)->child(0)->name == VARIABLE && opchild(1)->child(0)->name == VARIABLE &&
                opchild(0)->varchild(0)->value.val == opchild(1)->varchild(0)->value.val) {
                    VarNode* mul = opchild(1)->varchild(0); // reuse
                    changeOperator(MULTIPLY); // now a multiply operator
                    mul->changeOperator(CONSTANT); // reuse as a constant for exponent
                    mul->setVal(2); // exponent is 2
                    freechild(1); // free second op
                    setchild(1, mul); // add exponent
                    return nullptr;
            }
            
            break;
        }
        case SUBTRACT:{
            if(child(0)->name == CONSTANT && child(1)->name == CONSTANT){ // sub(c,c) = c
                varchild(0)->setVal(varchild(0)->value.val - varchild(1)->value.val);
                freechild(1,true);
                return child(0);
            }
            if(child(1)->name == CONSTANT){ // sub(o,c) = add(o,-c)
                varchild(1)->setVal(varchild(1)->value.val * VTYPE(-1));
                changeOperator(ADD);
                return simplify(); // new addition may simplify further
            }
            if(child(1)->name == NEGATIVE){ // sub(a,neg(b)) = add(a,b)
                r = opchild(1)->child(0);
                freechild(1);
                setchild(1, r);
                changeOperator(ADD);
                return simplify(); // new addition may simplify further
            }
            if(child(0)->name == NEGATIVE){ // sub(neg(a),b) = neg(add(a,b))
                OpNode *neg = opchild(0);
                // move up negate's child under me
                r = neg->child(0);
                setchild(0, r);
                // move negate above me
                neg->setchild(0, this);
                // update operator
                changeOperator(ADD);
                // r = simplify(); ... resimplification ideal but too complex with current impl.
                return (Node*)neg;
            }
            if(child(0)->name == CONSTANT && varchild(0)->value.val == 0){ // sub(0, a) = neg(a)
                me->swap();
                freechild(1,true); // remove constant
                setchild(1, nullptr); // prevent bad pointer
                changeOperator(NEGATIVE);
                return simplify();
            }
            if(child(0)->name == VARIABLE && child(1)->name == VARIABLE && varchild(0)->value.val == varchild(1)->value.val) { // sub(x,x) = 0
                child(0)->changeOperator(CONSTANT);
                varchild(0)->setVal(0);
                freechild(1, true);
                return child(0);
            }
            
            break;
        }
        case MULTIPLY:{
            if(child(0)->name == CONSTANT && child(1)->name == CONSTANT){ // mul(c,c) = c
                varchild(0)->setVal(varchild(0)->value.val * varchild(1)->value.val);
                freechild(1, true); // free second constant
                return child(0);
            }
            
            if(child(0)->name == MULTIPLY && child(1)->name == MULTIPLY) { // mul(mul(c,o),mul(c,o)) = mul(c,mul(o,o))
                if(opchild(0)->child(1)->name == CONSTANT) opchild(0)->swap();
                if(opchild(1)->child(1)->name == CONSTANT) opchild(1)->swap();

                if(opchild(0)->child(0)->name == CONSTANT && opchild(1)->child(0)->name == CONSTANT){
                    VarNode *ls = opchild(0)->varchild(0), *rs = opchild(1)->varchild(0);
                    ls->setVal(ls->value.val * rs->value.val);
                    // drop second old constant
                    rs->free();
                    // move leftside opnode into rightside add 
                    opchild(1)->setchild(0, opchild(0)->child(1));
                    // move summed constant up from leftside
                    freechild(0);
                    setchild(0, ls);
                }
                return nullptr;
            }
            if(child(0)->name == NEGATIVE && child(1)->name == NEGATIVE){ // mul(neg(a),neg(b)) = mul(a,b)
                OpNode *ls = opchild(0), *rs = opchild(1);
                setchild(0, ls->child(0));
                setchild(1, rs->child(0));
                ls->free();
                rs->free();
                return nullptr;
            }

            if(child(1)->name == CONSTANT){ // mul(a,c) = mul(c,a) move constants to the left side
                me->swap();
            }
            if(child(0)->name == CONSTANT){ // mul(c, )
                if(varchild(0)->value.val == VTYPE(0)){ // mul(0, x) = 0
                    freechild(1,true); // free op node and all children
                    return child(0);
                }
                if(varchild(0)->value.val == VTYPE(1)){ // mul(1, x) = x
                    freechild(0,true);
                    return child(1);
                }
                if(child(1)->name == MULTIPLY){ // mul(c,mul(c,o)) = mul(c,o)
                    if(opchild(1)->child(0)->name == CONSTANT){
                        varchild(0)->setVal(varchild(0)->value.val * opchild(1)->varchild(0)->value.val);
                        r = opchild(1)->child(1);
                        opchild(1)->freechild(0); // free constant
                        freechild(1); // free opnode
                        setchild(1, r);
                        return nullptr;
                    }
                }
                if(varchild(0)->value.val == VTYPE(-1)){ // mul(-1, x) = neg(x)
                    me->swap();
                    freechild(1, true);
                    changeOperator(NEGATIVE);
                    return simplify(); // simplify new negative operator
                }
            }
            if(child(1)->name == VARIABLE){ // mul(a,x) = mul(x,a) move variables to the left side
                me->swap();
            }
            if(child(0)->name == VARIABLE){ // mul(x, )
                if(child(1)->name == MULTIPLY){
                    if(opchild(1)->child(0)->name == VARIABLE){ // mul(x,mul(x,o)) = mul(pow(x,2),o)
                        r = opchild(1)->child(1); // save last inner operand

                        VarNode* exp = varchild(0); // use for exponent
                        exp->changeOperator(CONSTANT);
                        exp->setVal(2); // squared

                        OpNode* pow = opchild(1);
                        pow->changeOperator(POWER);
                        pow->setchild(1, exp); // set power

                        setchild(0, pow);
                        setchild(1, r);
                        return nullptr;
                    }
                }
                if(child(1)->name == DIVIDE){
                    if(opchild(1)->child(0)->name == VARIABLE){ // mul(x,div(x,o)) = div(pow(x,2),o)
                        //    div(pow(x,2),o)
                        OpNode* div = opchild(1);
                        VarNode* exp = div->varchild(0); // use for exponent
                        exp->changeOperator(CONSTANT);
                        exp->setVal(2); // squared
                        
                        OpNode* pow = static_cast<OpNode*>(this); // reuse self as the pow op
                        pow->changeOperator(POWER);
                        pow->setchild(1, exp); // set power

                        div->setchild(0, pow);
                        return div;
                    }
                }
            }

            if(child(0)->name == INVERSE && child(1)->name == INVERSE){ // mul(inv(a),inv(b)) = inv(mul(a,b))
                OpNode *ls = opchild(0);
                // move up inv children
                setchild(0, opchild(0)->child(0));
                r = opchild(1)->child(0); // get right side inner node
                freechild(1); // free rs op
                setchild(1, r);
                // move inv above me
                ls->setchild(0, this);
                return ls;
            }
            if(child(0)->name == INVERSE){
                me->swap();
            }
            if(child(1)->name == INVERSE){ // mul(a,inv(b)) = div(a,b)
                r = opchild(1)->child(0);
                freechild(1);
                setchild(1, r);
                changeOperator(DIVIDE);
                return nullptr;
            }

            if(child(0)->name == VARIABLE && child(1)->name == VARIABLE && varchild(0)->value.val == varchild(1)->value.val) { // mul(x,x) = pow(x,2)
                changeOperator(POWER);
                child(1)->changeOperator(CONSTANT);
                varchild(1)->setVal(2);
                return nullptr;
            }

            // mul(___(x) * ___(x)) = pow(___(x), 2)
            if( (child(0)->name == SIN || child(0)->name == COS || child(0)->name == TAN ||
                child(0)->name == ABS || child(0)->name == INVERSE || child(0)->name == NEGATIVE) &&
                child(1)->name == child(0)->name &&
                opchild(0)->child(0)->name == VARIABLE && opchild(1)->child(0)->name == VARIABLE &&
                opchild(0)->varchild(0)->value.val == opchild(1)->varchild(0)->value.val) {
                    VarNode* exp = opchild(1)->varchild(0); // reuse
                    changeOperator(POWER); // now a power operator
                    exp->changeOperator(CONSTANT); // reuse as a constant for exponent
                    exp->setVal(2); // exponent is 2
                    freechild(1); // free second op
                    setchild(1, exp); // add exponent
                    return nullptr;
            }

            // Trig Identities - Multiplication

            break;
        }
        case DIVIDE:{
            if(child(0)->name == CONSTANT && child(1)->name == CONSTANT){ // div(c,c) = c
                varchild(0)->setVal(varchild(0)->value.val / varchild(1)->value.val);
                freechild(1,true);
                return child(0);
            }
            if(child(0)->name == INVERSE){ // div(inv(a),b) = inv(mul(a,b))
                OpNode *inv = opchild(0);
                setchild(0, inv->child(0));
                inv->setchild(0, this);
                changeOperator(MULTIPLY);
                // r = simplify() // ideally simplify this further, but current impl. is too complex
                
                return inv;
            }
            if(child(1)->name == INVERSE){ // div(a,inv(b)) = mul(a,b)
                r = opchild(1)->child(0);
                freechild(1);
                setchild(1, r);
                changeOperator(MULTIPLY);
                return simplify();
            }
            if(child(0)->name == CONSTANT) {
                if(varchild(0)->value.val == 0) { // div(0,x) = 0
                    freechild(1, true);
                    return child(0);
                }
                if(varchild(0)->value.val == 1) { // div(1,x) = inv(x)
                    me->swap();
                    freechild(1, true); // free all
                    setchild(1, nullptr);
                    changeOperator(INVERSE);
                    return simplify();
                }
            }
            if(child(0)->name == VARIABLE && child(1)->name == VARIABLE && varchild(0)->value.val == varchild(1)->value.val) { // div(x,x) = 1
                child(0)->changeOperator(CONSTANT);
                varchild(0)->setVal(1);
                freechild(1,true);
                return child(0);
            }
            
            
            // div(mul(x,a),x) = a
            // div(mul(a,x),x) = a
            if(child(0)->name == MULTIPLY && child(1)->name == VARIABLE){
                for(int mulside=0; mulside < 2; ++mulside){ // loop for both sides (inside)
                    if(opchild(0)->child(mulside)->name == VARIABLE){
                        if(opchild(0)->varchild(mulside)->value.val == varchild(1)->value.val){
                            r = opchild(0)->child(!mulside);
                            opchild(0)->freechild(mulside); // remove the mul var
                            freechild(0); // remove both div children
                            freechild(1, true);
                            return r;
                        }
                    }
                }
            }

            // div(x,mul(x,a)) = inv(a)
            // div(x,mul(a,x)) = inv(a)
            if(child(0)->name == VARIABLE && child(1)->name == MULTIPLY){
                for(int mulside=0; mulside < 2; ++mulside){ // loop for both sides (inside)
                    if(opchild(1)->child(mulside)->name == VARIABLE){
                        if(opchild(1)->varchild(mulside)->value.val == varchild(0)->value.val){
                            OpNode* inv = opchild(1);
                            if(mulside == 0) inv->swap(); // get the important node to become child 0
                            inv->changeOperator(INVERSE); // change to inverse
                            inv->freechild(1); // remove the mul var
                            freechild(0, true); // remove first div child
                            return inv;
                        }
                    }
                }
            }
            

            // div(___(x),___(x))
            if( (child(0)->name == SIN || child(0)->name == COS || child(0)->name == TAN ||
                child(0)->name == ABS || child(0)->name == INVERSE || child(0)->name == NEGATIVE) &&
                child(1)->name == child(0)->name &&
                opchild(0)->child(0)->name == VARIABLE && opchild(1)->child(0)->name == VARIABLE &&
                opchild(0)->varchild(0)->value.val == opchild(1)->varchild(0)->value.val) {
                    VarNode* c = opchild(0)->varchild(0);
                    c->changeOperator(CONSTANT);
                    c->setVal(1);
                    freechild(0); // keep child alive
                    freechild(1, true);
                    return c;
            }

            // Trig Identities - Division
            if(child(0)->name == SIN && child(1)->name == COS &&
                opchild(0)->child(0)->name == VARIABLE && opchild(1)->child(0)->name == VARIABLE &&
                opchild(0)->varchild(0)->value.val == opchild(1)->varchild(0)->value.val) {
                    child(0)->changeOperator(TAN);
                    freechild(1,true);
                    return child(0);
            }

            break;
        }
        case POWER:{
            if(child(0)->name == CONSTANT && child(1)->name == CONSTANT){ // pow(c,c) = c
                varchild(0)->setVal( std::pow(varchild(0)->value.val, varchild(1)->value.val) );
                freechild(1,true);
                return child(0);
            }

            if(child(1)->name == CONSTANT) {
                if(varchild(1)->value.val == 0) { // pow(a,0) = 1
                    freechild(0,true);
                    varchild(1)->setVal(1);
                    return child(1);
                }
                if(varchild(1)->value.val == 1) { // pow(a,1) = a
                    freechild(1, true);
                    return child(0);
                }
            }
            if(child(0)->name == CONSTANT && varchild(0)->value.val == 1) { // pow(1,a) = 1
                freechild(1,true);
                return child(0);
            }
            if(child(0)->name == POWER) { // pow(pow(a,b),c) = pow(a,mul(c,b))
                OpNode* mul = opchild(0); // reuse for mul op
                mul->changeOperator(MULTIPLY);

                Node *base = mul->child(0); // save original base child
                mul->setchild(0, child(1)); // move c to replace a
                setchild(1, mul); // replace exponent with mul op
                setchild(0, base); // update the base with the lost base child

                // return simplify(); // for a better simplification, but can be expensive
                return nullptr;
            }
            if(child(0)->name == INVERSE) { // pow(inv(a),b)
                if(child(1)->name == CONSTANT){ // pow(inv(a),c) = pow(a,-c)
                    varchild(1)->setVal(varchild(1)->value.val * VTYPE(-1)); // invert constant
                    r = opchild(0)->child(0); // get internal node
                    freechild(0); // free the inv op
                    setchild(0, r); // update the base
                    return nullptr;
                } else { // pow(inv(a),b) = inv(pow(a,b))
                    r = opchild(0)->child(0); // get internal node
                    child(0)->setchild(0, this); // move inv op to the outside
                    setchild(0, r); // update the base
                    return parent; // return my new parent
                }
            }

            break;
        }
        case ABS:{
            if(child(0)->name == CONSTANT) {  // abs(c) = c
                varchild(0)->setVal(std::abs(varchild(0)->value.val));
                return child(0);
            }
            if(child(0)->name == ABS){ // abs(abs(a)) = abs(a)
                return child(0);
            }
            if(child(0)->name == NEGATIVE) { // abs(neg(a)) = abs(a)
                r = opchild(0)->child(0);
                freechild(0); // remove neg node
                return r;
            }

            break;
        }

        // Trigonometric Identities:
        case SIN:{
            if(child(0)->name == CONSTANT) {  // sin(c) = c
                varchild(0)->setVal(std::sin(varchild(0)->value.val));
                return child(0);
            }
            break;
        }
        case COS:{
            if(child(0)->name == CONSTANT) {  // cos(c) = c
                varchild(0)->setVal(std::cos(varchild(0)->value.val));
                return child(0);
            }
            break;
        }
        case TAN:{
            if(child(0)->name == CONSTANT) {  // tan(c) = c
                varchild(0)->setVal(std::tan(varchild(0)->value.val));
                return child(0);
            }
            break;
        }
    }

    return nullptr; // cannot simplify
}

Node::ReasonCode Node::validateNodeTree(RootNode* rootnode) {

    if(rootNode != rootnode) return ROOTNODE_BAD_MEMORY;
    if(rootNode->node == this && parent != nullptr) return ROOTNODE_BAD_PARENT;

    ReasonCode code = validateNode();
    if(code != SUCCESS) return code;

    for(int i=0; i < arity; ++i){
        code = child(i)->validateNodeTree(rootnode);
        if(code != SUCCESS) return code;
    }
    return SUCCESS;
}

/**
 *      Variable Node:
**/

void VarNode::ConstructVarNode(RootNode* rootnode, FunctionName fname, const Value& val, bool randomize) {
    ConstructNode(rootnode, fname);
    value = val;

    if((name == NONE && randomize) || name == RANDOM_VAR) { // no given function name
        name = (Random::chance(rootNode->params->constantChance) ? CONSTANT : VARIABLE);
    }


    if(!value.isSet && randomize) {
        randomizeValue();
    }

}

Node::ReasonCode VarNode::validateNode() {
    if(arity > 0) return ARITY;
    if(!value.isSet) return UNSET_VALUE;
    if(name != CONSTANT && name != VARIABLE) return INVALID_VARTYPE;
    if(parent != nullptr){
        if(parent->child(0) != this && parent->child(1) != this) return PARENTLINK;
    }
    return SUCCESS;
}

Value& VarNode::setVal(VTYPE val) {
    switch(name){
        case CONSTANT:{
            int sign = std::signbit(val) ? -1 : 1;
            if(std::abs(val) > rootNode->params->maxConstant){
                val = sign * INFINITY;
            }
            if(std::abs(val) < rootNode->params->minConstant){
                val = 0.f;
            }
            value = std::round( val * VTYPE(rootNode->params->decimalPlacesExp) ) / VTYPE(rootNode->params->decimalPlacesExp);
            break;
        }
        case VARIABLE:{
            value = size_t(val); // variables are an index of the function variable
            break;
        }
    }
    return value;
}

void VarNode::randomizeValue() {
    switch(name) {
        case CONSTANT: {
            setVal(Random::random() * rootNode->params->maxConstant * 2 - rootNode->params->maxConstant);
            break;
        }
        case VARIABLE: {
            setVal(Random::randomInt(rootNode->params->points.numVars - 1));
            break;
        }
    }
}

void VarNode::changeOperator(FunctionName name){
    this->name = name;
}

void VarNode::listOfNodes(NodeList& nodes) const {
    Node* t = static_cast<Node*>(const_cast<VarNode*>(this)); // convert to editable node type
    nodes += (name == CONSTANT ? NodeList {{}, {}, {t}, {t}} : NodeList {{}, {t}, {}, {t}});
}

Node* VarNode::copy(RootNode* newRootNode) const {
    VarNode* node = (VarNode*)Node::copy(newRootNode); // parent inherited - get new node
    node->setVal(value.val);
    return node;
}

VTYPE VarNode::compute(const Operators::Variables& vars) const {
    if(name == CONSTANT) return value.val; // constant resolves instantly
    if(size_t(value.val) >= vars.size()){
        warning("found variable index out of bounds: " + std::to_string(value.val) + " reaches beyond " + std::to_string(vars.size()));
        return VTYPE(0);
    }
    return vars[size_t(value.val)];
}

std::string VarNode::string() const{
    if(!value.isSet) return "N/A";
    return ( name == CONSTANT ? std::to_string(value.val) : std::string("var") + std::to_string(int(value.val)) );
}

std::string VarNode::form() const {
    return ( name == CONSTANT ? "const" : "var" );
}

void VarNode::cout(int level) const {
    std::string tabs(level*4, ' ');
    syslog::cout << tabs << "Type: " << FunctionNameString[name] << "\n"
              << tabs << "Value: " << string() << "\n";
    Node::cout(level);
}



/**
 *      Operator Node:
 **/

void OpNode::ConstructOpNode(RootNode* rootnode, FunctionName fname, const Children& _children, bool randomize) {
    ConstructNode(rootnode, fname, nullptr, _children);
    
    if((name == NONE && randomize) || name == RANDOM_OP) { // no given function name
        name = rootNode->params->operatorFunctions[ Random::randomInt(rootNode->params->operatorFunctions.size() - 1) ];
    }

    arity = (rootNode->params->operatorList[name]).arity;
    function = (rootNode->params->operatorList[name]).function;

    if(randomize){
        for(int i=0;i < arity;++i){
            if(child(i) == nullptr) setchild(i, rootNode->createNode(RANDOM_VAR, true)); // create a random node
        }
    }
}

OpNode::~OpNode() {
    
}

Node::ReasonCode OpNode::validateNode() {
    if(arity == 0) return ARITY;
    bool found = false;
    for(FunctionName n : rootNode->params->operatorFunctions){
        if(n == name){
            found = true;
            break;
        }
    }
    if(!found) return INVALID_OPTYPE;

    for(int i=0;i < arity; ++i){
        if(child(i) == nullptr) return i ? NULLCHILD_1 : NULLCHILD_0;
        if(child(i)->parent != this) return i ? CHILDLINK_1 : CHILDLINK_0;
    }
    if(parent != nullptr){
        if(parent->child(0) != this && parent->child(1) != this) return PARENTLINK;
    }
    return SUCCESS;
}

void OpNode::free() { // this method does NOT free its children
    setchild(0, nullptr);
    setchild(1, nullptr);
    Node::free();
}

void OpNode::freeAll() {
    for(int i=0;i < arity; ++i){
        freechild(i,true);
    }
    Node::free();
}

VTYPE OpNode::compute(const Operators::Variables& vars) const {
    switch(arity){
        case 1: return function(child(0)->compute(vars), 0);
        case 2: return function(child(0)->compute(vars), child(1)->compute(vars));
    }
    return 0; // bad operator compute
}

void OpNode::listOfNodes(NodeList& nodes) const {
    Node* t = static_cast<Node*>(const_cast<OpNode*>(this)); // convert to editable node type
    nodes += NodeList {{t}, {}, {}, {t}}; // add this operator
    
    // add children nodes
    for(int i=0;i < arity; ++i){
        children[i]->listOfNodes(nodes);
    }
}

void OpNode::changeOperator(FunctionName name){
    this->name = name;
    function = rootNode->params->operatorList[name].function;
    arity = rootNode->params->operatorList[name].arity;
}

void OpNode::swap() {
    Node* _tmp = children[0];
    children[0] = children[1]; // swap
    children[1] = _tmp; // swap
}

std::string OpNode::string() const {
    
    std::string s = FunctionNameString[name] + "(";
    
    for(int i=0; i<arity; i++){
        if(child(i) != nullptr){
            s += child(i)->string() + (i == arity-1 ? "" : ", ");
        } else
            s += std::string("null") + (i == arity-1 ? "" : ", ");
    }

    s += ")";

    return s;
}

std::string OpNode::form() const {
    
    std::string s = FunctionNameString[name] + "(";
    for(int i=0; i<arity; i++){
        if(child(i) != nullptr){
            s += child(i)->form()
                 + (i == arity-1 ? "" : ", ");
        }
    }
    s += ")";

    return s;
}

void OpNode::cout(int level) const {
    std::string tabs(level*4, ' ');
    syslog::cout << tabs << "Operator: " << string() << "\n";
    Node::cout(level);
}