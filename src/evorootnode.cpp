#define REQUIRE_NODEPOOL
#include "evorootnode.h"

using namespace NodeTypes;

std::atomic<size_t> NodePool::totalCount(0); // tracker for all nodes

NodePool::NodePool(): count(0) {
    freeNode = preAllocateMemoryPool(); // preallocate first pool of memory - first empty node is returned
}

NodePool::~NodePool() {
    for(Pool* p : memPool){
        delete p; //sweeping up all pool memory
    }
}

uNode* NodePool::preAllocateMemoryPool() {
    curMemPool = new Pool; // raw sub pool allocation
    memPool.push_back(curMemPool); // add the new sub-pool to the significant memory pool
    uNode* node = &curMemPool->at(0); // this is the address to the first node in the new pool

    // pre-populate the fresh double-linked list with valid in-order pointers (with nullptr ends)
    for(size_t i = 0; i < curMemPool->size(); ++i){
        uNode *next, *prev;
        next = (i == curMemPool->size()-1 ? nullptr : &(*curMemPool)[i+1]);
        prev = (i == 0 ? nullptr : &(*curMemPool)[i-1]);

        (*curMemPool)[i].n = next;
        (*curMemPool)[i].p = prev;
    }

    return node;
}

void NodePool::deallocate_Node(Node* removeNode)  { // remove a node that must be in the given nodepool
    std::scoped_lock lock(tsafe);
    uNode* unode = removeNode->memory;
    if(unode->n != freeNode){
        if(unode->n != nullptr)
            unode->n->p = unode->p; // collapse double linked list
        if(unode->p != nullptr)
            unode->p->n = unode->n; // collapse double linked list

        unode->p = freeNode->p; // update previous node to last node allocated
        if(unode->p != nullptr) // check if this is the first node in the system
            unode->p->n = unode; // update next node on last allocated node

        unode->n = freeNode; // update next node to the older next free node
        unode->n->p = unode; // update previous node on the old next free node
    }
    freeNode = unode; // the next free node is now set to the current deallocated node
    count--; // track node allocation
    totalCount--;
};

bool NodePool::addLinkNewPool(uNode* lastNode) { // pass the last node in the previous pool, and free node will be autoset to the next free node in a new pool
    freeNode = preAllocateMemoryPool(); // update freenode to the first node in the fresh memory pool (it's so clean!)
    if(freeNode == nullptr) return false; // failed to allocate

    lastNode->n = freeNode; // link the new memory pool
    freeNode->p = lastNode; // link the previous memory pool
    /*   ...  [ e next-> [Y0]] } (X) <link sub pools> (Y) { [ [Xe] <-prev 0 ] ...   */

    return true;
}

OpNode* NodePool::allocate_OpNode() { // Instantiate the OpNode() in the array at position freeNode then increment freeNode pointer to the next node in the double linked list
    std::scoped_lock lock(tsafe);
    if(freeNode == nullptr){
       throw std::runtime_error("Node allocation failed causing a critical failure");
    }
    freeNode->node.emplace<OpNode>(OpNode()); // emplace the node in the memory pool
    OpNode* rval = &std::get<OpNode>(freeNode->node); // get the address of the new node instantiated in memory
    if(freeNode == nullptr) {
        warning("freeNode is empty");
    }
    rval->memory = freeNode;
    freeNode = freeNode->n; // increment the next free node
    count++; // track node allocation
    totalCount++;
    if(count >= curMemPool->size() || freeNode == nullptr){ // reached maximum pool size
        addLinkNewPool(rval->memory); // automatically allocate and link the next sub-pool
    }
    return rval;
}

VarNode* NodePool::allocate_VarNode() { // Instantiate the VarNode() in the array at position freeNode then increment freeNode pointer to the next node in the double linked list
    std::scoped_lock lock(tsafe);
    if(freeNode == nullptr){
       throw std::runtime_error("Node allocation failed causing a critical failure");
    }
    freeNode->node.emplace<VarNode>(VarNode()); // emplace the node in the memory pool
    VarNode* rval = &std::get<VarNode>(freeNode->node); // get the address of the new node instantiated in memory
    if(freeNode == nullptr) {
        warning("freeNode is empty");
    }
    rval->memory = freeNode;
    freeNode = freeNode->n; // increment the next free node
    count++; // track node allocation
    totalCount++;
    if(count >= curMemPool->size()){ // reached maximum pool size
        addLinkNewPool(rval->memory); // automatically allocate and link the next sub-pool
    }
    return rval;
}

const Parameters* RootNode::params = nullptr; // static pointer for root node parameters

RootNode::RootNode(): score(INFINITY), complexity(0), node(nullptr), lock(new std::mutex), form("") {} // defualt initialization of root node

RootNode::~RootNode() {
    delete lock; // free mutex
    node->freeAll(); // free all nodes after completed
}

int RootNode::validateNodeTree() {
    size_t treeSize = 0,
           calcPoolSize = 0,
           poolSize = pool.getNodeCount(),
           maximumPoolSize = 0;

    for(NodePool::Pool* p : pool.memPool) maximumPoolSize += p->size(); /* raw calculation of the maximum pool size
        (sub pools should always have the same size, but even so let's just calculate individually anyway. Kodi: *Woof!*) */

    if(node != nullptr){
        NodeList list;
        node->listOfNodes(list);
        treeSize = list.all.size();
    }
    {
        uNode* unode = nullptr;
        for(NodePool::Pool* memPool : pool.memPool){
            for(uNode& nn : *memPool){
                if(&nn == pool.freeNode){
                    unode = &nn;
                    break;
                }
            }
            if(unode == nullptr) continue; // this pool is full - try next pool
        }
        if(unode == nullptr){
            syslog::cout << "found nodepool with no valid freenode\n";
            return false;
        }
        while(unode != nullptr) {
            unode = unode->p; // iterate backwards until we reach the first node
            if(calcPoolSize++ == maximumPoolSize){
                syslog::cout << "reached maximum pool size when searching for first node\n";
            }
        }
        calcPoolSize--; // compensate the freenode that we added
    }

    if(poolSize != calcPoolSize){
        syslog::cout << "Fatal calculation error: poolSize=" << poolSize << " != calcPoolSize=" << calcPoolSize << "\n";
    }

    return (calcPoolSize - treeSize);
}

// This will parse the given string and generate a node tree to overwrite the given root node with
bool RootNode::parseRootNodeString(const std::string& data) {
    if(!data.size()) return false; // no length do nothing
    size_t pos = 0;
    if(node != nullptr) node->freeAll(); // free old node
    node = nullptr;

    Node* newNode = StringParser::parseIteration(*this, data, pos);
    if(newNode == nullptr) return false;

    node = newNode;
    return true;
}

// This calculates the form of the node tree into a string and updates the form string
void RootNode::calculateForm() {
    form = node->form();
}

// Compute the root tree with a given range of data points - updates given equation points given with the results and point range
void RootNode::computeEquation(Operators::EqPoints& data, double from, double to, double precision) {
	if(data.points.size() || data.results.size()) return; // do not calculate if non-empty given points
    for(double i=from; i<to; i+=precision){
		data.points.push_back({}); // add new variable range to data points
        data.points.back().resize(data.numVars, i); // allocate point range for all variables - side note: this will affect the visual result greatly when using multi-variable equations
	}
    for(const Operators::Variables& v : data.points){
        data.results.push_back( node->compute(v) ); // compute all points and update results
    }
}

// Call this simplify method for simplifying from a root - (may add a root parent if needed)
void RootNode::simplify(Node* parent) {
    do {
        Node* root = node->simplify(); // get new root node
        if(root != nullptr){ // if new root node
            if(node->parent == parent) node->free(); // free original root node if no longer used - parent is typically nullptr
            node = root; // update root node
            node->setParent(parent); // update the root's parent - this is usually nullptr and could cause problems if not
            continue; // re-simplify until nullptr
        }
        break; // don't loop
    } while(1);
}

Node* RootNode::allocateOpNode(NodeTypes::FunctionName name, const Children& children, bool randomize){
    static std::mutex threadsafe; std::scoped_lock lock(threadsafe);

    OpNode* n = pool.allocate_OpNode();
    n->ConstructOpNode(this, name, children, randomize);

    return n;
}

Node* RootNode::allocateVarNode(NodeTypes::FunctionName name, const Value& value, bool randomize){
    static std::mutex threadsafe; std::scoped_lock lock(threadsafe);
    
    VarNode* n = pool.allocate_VarNode();
    n->ConstructVarNode(this, name, value, randomize);

    return n;
}

// This is a method for generating a new node into memory: randomize will determine if the value/children are randomly set
Node* RootNode::createNode(NodeTypes::FunctionName name, bool randomize) {

    if(name == RANDOM_OP){
        return allocateOpNode(name, {nullptr, nullptr}, randomize);
    }
    if(name == RANDOM_VAR){
        return allocateVarNode(name, NOVALUE, randomize);
    }

    // UPDATE: Creating a node with NONE type is illegal
    if(name == NONE){ // random creation
        throw std::runtime_error("Contact Developer -> NONE is a deprecated type to instantiate");
    } else {
        // preset creation
        if(name == CONSTANT || name == VARIABLE) return allocateVarNode(name, NOVALUE, randomize);
        return allocateOpNode(name, {nullptr, nullptr}, randomize);
    }
}

// Mutate Change - Call on root node and set root node to return value
Node* RootNode::mutateChange(int numMutations) {
    const size_t len = params->operatorFunctions.size(); // get num operators

    Node* root = node;

    while(numMutations-- > 0){

        NodeList nodes; root->listOfNodes(nodes); // get fresh list of nodes
        if(!nodes.all.size()) return root; // if no nodes were found - nothing to do

        Node* self = nodes.all[Random::randomInt(nodes.all.size() - 1)], // pick a random node in the tree
            * newNode = self; // new node to be used - default is itself

        // new name for new mutation node
        FunctionName newName = (Random::chance(params->operatorChance) ?   // the chance a node is mutate-changed into an operator
                params->operatorFunctions[Random::randomInt(len - 1)] // pick random operator
                    : (Random::chance(params->constantChance) ? CONSTANT : VARIABLE) // pick constant or variable randomly
        );

        bool newVarNode = (newName == CONSTANT || newName == VARIABLE); // quick check if new node is varnode or opnode
        bool freeMe = false; // determines if the old node is to be freed from memory
        OpNode* p = (OpNode*)self->parent; // get original parent

        if(self->name == CONSTANT || self->name == VARIABLE) { // if self is varnode
            VarNode* me = (VarNode*)self;
            if(newVarNode){ // reuse myself as the new varnode
                me->name = newName;
                me->randomizeValue();
            } else {
                OpNode* _node = (OpNode*)createNode(newName); // creates the new op node (_ do not shadow)

                me->name = Random::chance(50) ? CONSTANT : VARIABLE; // reuse myself as the first child of the new op
                me->randomizeValue();
                _node->setchild(0, me);

                for(int i=1;i < _node->arity;++i){ // for the rest of the operands add new var nodes
                    if(_node->child(i) == nullptr) _node->setchild(i, createNode(RANDOM_VAR, true)); // create a random var node
                }

                newNode = _node;
            }
        } else { // if self is opnode
            OpNode* me = (OpNode*)self;
            if(newVarNode){
                newNode = createNode(newName, true); // random varnode
                freeMe = true; // destroy myself
            } else {
                int8_t prevArity = me->arity;
                me->changeOperator(newName);
                if(prevArity > me->arity){ // less operands - destroy
                    for(int i=me->arity;i < prevArity;++i){
                        me->freechild(i, true); // free the lost children
                    }
                } else { // more operands - create
                    for(int i=0;i < me->arity;++i){ // for the rest of the operands add new var nodes
                        if(me->child(i) == nullptr) me->setchild(i, createNode(RANDOM_VAR, true)); // create a random var node
                    }
                }
            }
        }
        if(newNode != self){ // if a node is reallocated
            if(self == root || p == nullptr){  // check if newNode is guaranteed to be root
                root = newNode; // update the root to the new node
            } else { // a child node is being re-allocated
                for(int i=0;i < p->arity; ++i){
                    if(self == p->child(i)) { p->setchild(i, newNode); break; } // update parent's child to the reallocated node
                }
            }
        }

        if(freeMe) self->freeAll(); // destroy the node if it is no longer used
        root->parent = nullptr;
    }

    return root;
}

// Mutate Add - Call on root node and set root node to return value
Node* RootNode::mutateAdd(int numMutations) {

    Node* root = node;
    if(root == nullptr) return nullptr;

    while(numMutations-- > 0){
        NodeList nodes; root->listOfNodes(nodes); // get fresh list of nodes
        if(!nodes.all.size()) return root; // if no nodes were found - nothing to do

        Node* self = nodes.all[Random::randomInt(nodes.all.size() - 1)]; // pick a random node in the tree
        OpNode* newNode = // new node to be used
            (OpNode*)createNode(RANDOM_OP); // create opnode with random operator, but not random operands
        OpNode* p = (OpNode*)(self->parent);
        
        newNode->setchild(Random::randomInt(newNode->arity - 1), self); // put new node on top of self/selected node
        
        for(int i=0;i < newNode->arity;++i){ // for the rest of the operands of new node, add a new varnode
            if(newNode->child(i) == nullptr) newNode->setchild(i, createNode(RANDOM_VAR, true)); // create a random var node
        }

        if(self == root || p == nullptr){ // check if newNode is guaranteed to be root
            root = newNode;
        } else { // newNode is not a root node - so must update the old parent's child to the new node
            for(int i=0;i < p->arity; ++i){ // make sandwich (parent points to newNode points to children) (p -> newNode -> {child...})
                if(p->child(i) == self) { p->setchild(i, newNode); break; } // update parent's child to the new allocated node
            }
        }

        root->parent = nullptr;
    }

    return root;
}

// Mutate Remove - Call on root node and set root node to return value
Node* RootNode::mutateRemove(int numMutations) {
    Node* root = node;

    while(numMutations-- > 0){
        NodeList nodes; root->listOfNodes(nodes); // get fresh list of nodes

        if(!nodes.operators.size()) return root; // if no nodes were found - nothing to do

        OpNode* self = (OpNode*)(nodes.operators[Random::randomInt(nodes.operators.size() - 1)]); // pick a random opnode in the tree
        int8_t idx = Random::randomInt(self->arity - 1);
        Node* replaceNode = self->child(idx),
            * p = self->parent;
        for(int i=0; i < self->arity; ++i){
            if(i == idx) continue;
            self->freechild(i, true); // kill the loose children that were not selected to become parent
        }
        if(self == root || p == nullptr){ // check if replacement node is guaranteed to be root
            root = replaceNode; // update root to the replacement node
        } else { // replacement will not be root
            for(int i=0;i < p->arity; ++i){
                if(p->child(i) == self) { p->setchild(i, replaceNode); break; } // update the parent's child node to the replacement node
            }
        }

        self->free(); // just free the node that was replaced so we don't kill the children
        root->parent = nullptr;
    }

    return root;
}

// Find Replace - Call on root node and set root node to return value - Find a var node variable and replace with a node tree
Node* RootNode::findReplace(size_t varnum, const Node* replaceWith) {
    Node* root = node;
    if(root == nullptr) return nullptr;

    NodeList nodes; root->listOfNodes(nodes); // get fresh list of nodes
    if(!nodes.all.size()) return root; // if no nodes were found - nothing to do

    for(Node* n : nodes.variables){
        VarNode* v = static_cast<VarNode*>(n);
        if(v->name == VARIABLE && v->value.val == varnum){
            Node* newNode = replaceWith->copy(this); // new node tree from copy
            if(v == root){
                root = newNode;
            } else {
                OpNode* p = static_cast<OpNode*>(v->parent);
                for(int i=0;i < p->arity; ++i){
                    if(p->child(i) == v) { p->setchild(i, newNode); break; } // update the parent's child node to the replacement node
                }
            }
            v->freeAll();
        }
    }

    root->parent = nullptr;
    return root;
}
