#ifndef __NODE_H_
#define __NODE_H_

// pre-declare Node classes
class NodePool;
class Node;
class VarNode;
class OpNode;
struct NodeList;
struct Value;

typedef Node* Children[2];

#include "evorootnode.h"
#include "nodetypes.h"
#include "operators.h"
#include "parameters.h"
#include "fitness.h"
#include "random.h"
#include "debug.h"
#include "hexstring.h"
#include "valuetype.h"


#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cstring>
#include <cmath>
#include <array>
#include <stdint.h>


#define WARNING_BAD_CALL(x) warning("Calling "#x" method without an OpNode!")

struct NodeList {
    std::vector<Node*> operators, variables, constants, all;

    NodeList& operator+=(const NodeList& rs);

};

class Node {
protected:
    Node(); // null initialization
	void ConstructNode(RootNode* rootnode, NodeTypes::FunctionName name=NodeTypes::NONE, Node* parent=nullptr, const Children& children={nullptr, nullptr}, int8_t arity=0);

    friend class RootNode; // Root node must be able to access all private members
    friend class NodePool; // Root node must be able to access all private members
public:

    enum ReasonCode {
        INVALID_VARTYPE, INVALID_OPTYPE,
        UNSET_VALUE, ROOTNODE_BAD_MEMORY,
        ROOTNODE_BAD_PARENT,
        ARITY, NULLCHILD_0, NULLCHILD_1,
        CHILDLINK_0, CHILDLINK_1, PARENTLINK,
        SUCCESS
    };

    static std::atomic<size_t> debugSkippedSimplify;

    uNode* memory; // pointer to the double linked list memory managed node space
    Node* parent;
    RootNode* rootNode;
    NodeTypes::FunctionName name;

	Children children; // contains children nodes
	int8_t arity;   // number of operators
	//double complexity; // complexity of the sub-tree structure

	virtual ~Node();
    
    virtual ReasonCode validateNode() = 0;
    ReasonCode validateNodeTree(RootNode* rootnode);
    
    virtual void free();
    virtual void freeAll();

    // an internal use - used by both fitness scoring and regular scoring
    static float rmsCalculate(const std::vector<VTYPE>& actual, const std::vector<VTYPE>& results);
    
    // testing and external use - disabled for offical use
    //static OpNode* addNode(NodeTypes::FunctionName name=NodeTypes::NONE, const Children& children={nullptr, nullptr}); // add an OpNode
    //static VarNode* addNode(NodeTypes::FunctionName name=NodeTypes::NONE, const Value& val=Value()); // add a VarNode

    virtual inline Node* child(int idx) const { WARNING_BAD_CALL("child"); return nullptr; }
    virtual inline OpNode* opchild(int idx) const { WARNING_BAD_CALL("opchild"); return nullptr; }
    virtual inline VarNode* varchild(int idx) const { WARNING_BAD_CALL("varchild"); return nullptr; }
    virtual inline void setchild(int idx, Node* c) { WARNING_BAD_CALL("setchild"); }
    virtual inline void freechild(int idx, bool all=false) { WARNING_BAD_CALL("freechild"); }

    /*
    inline bool operator>(const Node& rs) const { return complexity > rs.complexity; }
    inline bool operator<(const Node& rs) const { return complexity < rs.complexity; }
    inline bool operator>=(const Node& rs) const { return complexity >= rs.complexity; }
    inline bool operator<=(const Node& rs) const { return complexity <= rs.complexity; }
    inline bool operator==(const Node& rs) const { return complexity == rs.complexity; }
    */

    double computeComplexity();
    void setParent(Node* parent);
    void updateLinks();


    virtual Node* copy(RootNode* newRoot) const;
    virtual Node* copyMutate(RootNode* newRoot, const Node* to, const Node* from) const; // copy a node with a branch mutation on from-node to to-node
    virtual void changeOperator(NodeTypes::FunctionName fname) = 0; // all children must have a change operator
    virtual void listOfNodes(NodeList& list) const;

    virtual VTYPE compute(const Operators::Variables& vars) const = 0; // all children must have a compute method
    virtual float score(const Operators::EqPoints& points=Parameters::Params()->points, bool evo=false);

    Node* simplify();

    virtual std::string string() const = 0; // all children need a convert-to-string method
    virtual std::string form() const = 0; // all children need a convert-to-string method
	virtual void cout(int level=0) const;
};


class VarNode : public Node {
    VarNode() {} // null initialization
protected:
    void ConstructVarNode(RootNode* rootnode, NodeTypes::FunctionName name=NodeTypes::NONE, const Value& val=NOVALUE, bool randomize=true);

    friend class Node;
    friend class RootNode; // Root node must be able to access all private members
    friend class NodePool; // Root node must be able to access all private members
public:
    Value value;
    virtual ~VarNode() = default;

    virtual ReasonCode validateNode();

    Value& setVal(VTYPE val);
    void randomizeValue();

    virtual void changeOperator(NodeTypes::FunctionName name);

    virtual Node* copy(RootNode* newRoot) const;
    virtual void listOfNodes(NodeList& nodes) const;

    virtual VTYPE compute(const Operators::Variables& vars) const;

    virtual std::string string() const;
    virtual std::string form() const;
    virtual void cout(int level=0) const;
};



class OpNode : public Node {
    OpNode() {} // null initialization
protected:
    void ConstructOpNode(RootNode* rootnode, NodeTypes::FunctionName name=NodeTypes::NONE, const Children& children = {nullptr, nullptr}, bool randomize=true);

    friend class Node;
    friend class RootNode; // Root node must be able to access all private members
    friend class NodePool; // Root node must be able to access all private members
public:
    Operators::func function;
    
    virtual ~OpNode();

    virtual ReasonCode validateNode();


    void swap();
    virtual void free();
    virtual void freeAll();

    virtual inline Node* child(int idx) const { return children[idx]; }
    virtual inline OpNode* opchild(int idx) const { return static_cast<OpNode*>(children[idx]); }
    virtual inline VarNode* varchild(int idx) const { return static_cast<VarNode*>(children[idx]); }
    virtual inline void setchild(int idx, Node* c) { children[idx] = c; if(c != nullptr) c->setParent(this); }
    virtual inline void freechild(int idx, bool all=false) { if(children[idx] != nullptr){ if(all) children[idx]->freeAll(); else children[idx]->free(); children[idx] = nullptr; } }


    virtual void changeOperator(NodeTypes::FunctionName name);
    virtual std::string string() const;
    virtual std::string form() const;

    virtual void listOfNodes(NodeList& nodes) const;

    virtual VTYPE compute(const Operators::Variables& vars) const;

    virtual void cout(int level=0) const;
};


#endif 		// __NODE_H_
