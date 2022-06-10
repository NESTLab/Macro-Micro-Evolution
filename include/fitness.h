#ifndef __FITNESS_H__
#define __FITNESS_H__

#include "evorootnode.h"
#include "operators.h"
#include "nodetypes.h"
#include "node.h"
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>


class Fitness {
public:

    struct NodeScore {
        float score;
        std::vector<VTYPE> constants;
        Node* rcopy;
        NodeList* rcList;
    };

    typedef std::vector<NodeScore> Population;

    const Parameters* params;
    Node* root, *cpRoot;
    Population population;
    NodeList& rList;

    Operators::EqPoints data;

    Fitness(RootNode* rootnode, const Operators::EqPoints& odata, NodeList& rval);
    virtual ~Fitness();
    
    void syncConstants(NodeScore& rt, bool toorig=false);
    void sortPopulation();
    void mutateCrossPopulation(int startFrom=0);
    void mutateChangePopulation(int startFrom=0);
    void updateScorePopulation(int startFrom=0);

    void mutateCross(NodeScore& rt, NodeScore& frt, NodeScore& frt_b);
    void mutateChange(NodeScore& rt);
    void updateScore(NodeScore& rt);
    
    /*
    void workMutation(size_t i, size_t popSize, uint32_t cutoff);
    void threadedIterator(uint32_t cutoff);
    */

    float run();


};





#endif //__FITNESS_H_