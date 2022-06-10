#ifndef __EVO_ROOT_NODE__
#define __EVO_ROOT_NODE__

class RootNode;
class NodePool;
struct uNode;

#ifdef REQUIRE_NODEPOOL
#include "nodepool.h"
#endif

#include "node.h"
#include "nodetypes.h"
#include "parameters.h"
#include "stringparser.h"

#include <variant>
#include <list>
#include <array>
#include <mutex>
#include <atomic>

class NodePool { // custom memory pool manager
public:
    typedef std::array<uNode, 48> Pool; // define the cache pool size - 48 nodes should be sufficient for lower complexity values - this is a sub-pool and will be automatically allocated based on usage

private:
    uNode* freeNode;
    std::list<Pool*> memPool; // list of all memory sub-pools
    Pool* curMemPool; // points to the current memory sub-pool in use (this should be the last memory sub-pool that was allocated)
    std::mutex tsafe;
    std::atomic<size_t> count;
    static std::atomic<size_t> totalCount;

public:

    uNode* preAllocateMemoryPool();
    bool addLinkNewPool(uNode* lastNode);

    void deallocate_Node(Node* removeNode);

    OpNode* allocate_OpNode();
    VarNode* allocate_VarNode();

    inline size_t getNodeCount() { return count; }
    inline static size_t getTotalNodeCount() { return totalCount; }

    NodePool();
    virtual ~NodePool();

    friend class RootNode; // this is for the validateNodeTree() to access private pool information

};

class RootNode {
public:
    float score, complexity;
    Node* node;
    const Parameters* params;
    std::mutex* lock;
    std::atomic_bool complete;
    std::string form;
    NodePool pool;

    RootNode(const Parameters* params);
    virtual ~RootNode();


    void computeEquation(Operators::EqPoints& data, double from, double to, double precision=0.01); // compute and get results
    void calculateForm(); // calculate form for node tree
    void simplify(Node* parent=nullptr);

    // parse the given string and generate a node tree to overwrite the given root node with
    bool parseRootNodeString(const std::string& data);

    int validateNodeTree();

    Node* allocateOpNode(NodeTypes::FunctionName name, const Children& children, bool randomize);
    Node* allocateVarNode(NodeTypes::FunctionName name, const Value& value, bool randomize);
    Node* createNode(NodeTypes::FunctionName name=NodeTypes::NONE, bool randomize=false);

    Node* mutateAdd(int numMutations); // mutation add - call on root - return root node
    Node* mutateRemove(int numMutations); // mutation remove - call on root - return root node
    Node* mutateChange(int numMutations); // mutation change - call on root - return root node

    Node* findReplace(size_t varnum, const Node* replaceWith);
};



#endif // __EVO_ROOT_NODE__