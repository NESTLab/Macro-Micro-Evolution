#ifndef __EVOALGO_H__
#define __EVOALGO_H__


#include "visualevo.h"
#include "evorootnode.h"
#include "parameters.h"
#include "operators.h"
#include "nodetypes.h"
#include "clock.h"
#include "debug.h"
#include "node.h"
#include "calculatepoolsize.h"
#include "stringparser.h"

#include <numeric>
#include <variant>
#include <limits>

class EvoAlgo {
public:

    typedef std::vector<RootNode*> Population;
    typedef void (*Worker)(EvoAlgo* _this, size_t i, size_t popSize, size_t spread, void* extra); // function pointer for our thread genreator worker

    std::weak_ptr<VisualEvo::Graph> graph;

    const Parameters* params;
    Population population;
    Population shadowPopulation; // allocated space for temporary rootnode modification/calculations

    const Operators::EqPoints& data;
    int generation, drawGraphCount;

    std::vector<float> scoreDatabase; // previous scores

    EvoAlgo(const Parameters* params=Parameters::Params(), const Operators::EqPoints& data=Parameters::Params()->points);
    virtual ~EvoAlgo();
    
    static void workRootNodeAllocator(EvoAlgo* _this, size_t i, size_t popSize, size_t spread, void* extra);
    static void workSimplifyScoreComplexity(EvoAlgo* _this, size_t i, size_t popSize, size_t spread, void* extra);
    static void workScore(EvoAlgo* _this, size_t i, size_t popSize, size_t spread, void* extra);
    static void workFitness(EvoAlgo* _this, size_t i, size_t popSize, size_t spread, void* extra);
    static void workRepopulate(EvoAlgo* _this, size_t i, size_t popSize, size_t spread, void* extra);
    void threadGenerator(size_t start, size_t stop, Worker worker, void* extra=nullptr);


    void mutate(RootNode& rt, int iters);
    void sortPopulation();
    void repopulate();
    bool iteration();

    void drawGraph(RootNode& rt);

    bool checkForMemoryConsistency();

    void run();
};







#endif //__EVOALGO_H__