#ifndef __NODE_POOL_H__
#define __NODE_POOL_H__

#include "node.h"
#include "evorootnode.h"
#include <variant>

class NodePool;

struct uNode {  // double linked list node for memory management
    uNode *p, *n;
    std::variant<std::monostate, OpNode, VarNode> node;
};


#endif // __NODE_POOL_H__