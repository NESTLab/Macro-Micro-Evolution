#include "fitness.h"

Fitness::Fitness(RootNode* rootnode, const Operators::EqPoints& odata, NodeList& rval): params(rootnode->params), root(rootnode->node), rList(rval) {
    // Get root operator list of nodes
    root->listOfNodes(rList);

    // Copy data from original equation input data
    data.numVars = odata.numVars;
    data.results = odata.results;
    data.points = odata.points;

    // Get randomized subset of point values (sampleSize sets the percentage)
    const size_t sz = data.results.size(),
                 len = sz - std::round(params->fitness.sampleSize * float(sz));
    if(len > sz) throw std::runtime_error("Invalid sample size ratio");

    for(size_t i=0; i < len; ++i){ // iterate to remove random samples until sample size it met
        size_t pos = Random::randomInt(sz - 1 - i);
        data.results.erase(data.results.begin() + pos);
        data.points.erase(data.points.begin() + pos);
    }

    // Get population of the constants within the root
    std::vector<VTYPE> def; def.resize(rList.constants.size(), 0); // construct a default list of empty constants
    for(size_t i=0;i < rList.constants.size(); ++i){
        def[i] = ((VarNode*)rList.constants[i])->value.val; // copy constant values into the default constants list
    }
    population.resize(params->fitness.popSize, {INFINITY, def, nullptr, nullptr}); // construct an empty population

    // For now, the root node copy is null - making this a copy of the root node will allow for multi-threading support during population scoring
    //population.resize(params->fitness.popSize, {INFINITY, def, nullptr, nullptr, nullptr, false}); // construct an empty population with a root copy
    
    for(NodeScore& pop : population){ // root copy preparation - for all the root copies, prepare the copy's list of nodes
        //pop.rcopy = root->copy(); - do not use multithreading within the fitness function
        if(pop.rcopy != nullptr){
            pop.rcList = new NodeList;
            pop.rcopy->listOfNodes(*pop.rcList); // populate the list of nodes within all the root copies
        }
        //pop.lock = new std::mutex();
    }

    population[0].score = (population[0].rcopy == nullptr ?
                          root->score(data) : population[0].rcopy->score(data)); // update unchanged constants generation score
    mutateChangePopulation(1); // mutate change the rest of the constants for first generation
    updateScorePopulation(1); // calculate score for the rest of the mutated constants for first generation

    sortPopulation(); // sort the first generation
}

Fitness::~Fitness() {
    for(NodeScore& pop : population){
        if(pop.rcopy != nullptr){
            pop.rcopy->freeAll();
            delete pop.rcList;
        }
        /*
        if(pop.lock != nullptr){
            delete pop.lock;
        }
        */
    }
}

void Fitness::sortPopulation() {
    std::sort(population.begin(), population.end(), [](NodeScore& l, NodeScore& r) {
        return (l.score < r.score); // sort population with best scores first to last
    });
}


void Fitness::mutateCross(NodeScore& rt, NodeScore& frt_a, NodeScore& frt_b) {
    float rval = Random::random();
    int cross = 1; // determines if a cross mutation must still happen

    for(int i=0; i < rt.constants.size(); ++i){
        if(Random::random() <= rval){ // random cross mutation
            rt.constants[i] = frt_a.constants[i];
            cross = 0;
        } else { // copy from a better node
            rt.constants[i] = frt_b.constants[i];
        }
    }
    if(cross){ // must provide at least 1 cross mutation
        cross = Random::randomInt(rt.constants.size() - 1);
        rt.constants[cross] = frt_a.constants[cross];
    }
}

void Fitness::mutateChange(NodeScore& rt) {
    for(VTYPE& c : rt.constants){ // iterate all constants
        if(Random::chance(params->fitness.changeChance) && !std::isinf(c)){ // check chance that a constant will mutate
            c += c * (Random::random()-0.5); // mutate the constant upto half itself
        }
    }
}

void Fitness::updateScore(NodeScore& rt) {
    syncConstants(rt); // update the physical node's constants with the new constant data
    rt.score = (rt.rcopy == nullptr ? // check if using a root copy
                root->score(data) : rt.rcopy->score(data)); // calculate RMS score
}

void Fitness::mutateChangePopulation(int startFrom) {
    for(size_t i=startFrom; i < population.size(); ++i){
        mutateChange(population[i]);
    }
}

void Fitness::mutateCrossPopulation(int startFrom) {
    for(size_t i=startFrom; i < population.size(); ++i){
        int a = Random::randomInt( startFrom ), b = Random::randomInt(startFrom - 1);
        if(b >= a) ++b; // give all nodes a chance at selection

        mutateCross(population[i], population[a], population[b]);
    }
}

void Fitness::updateScorePopulation(int startFrom) {
    for(size_t i=startFrom; i < population.size(); ++i){
        updateScore(population[i]);
    }
}

void Fitness::syncConstants(NodeScore& rt, bool toorig) {
    NodeList& list = (rt.rcopy == nullptr || toorig ? // check if using the root copy node
                      rList : *rt.rcList);
    for(size_t i=0; i < list.constants.size(); ++i){
        ((VarNode*)(list.constants[i]))->setVal(rt.constants[i]);
    }
}


float Fitness::run() {
    if(population[0].constants.size() > 1){
        uint32_t cutoff = std::round(params->fitness.popSize * params->fitness.cutOff),
                repeat = params->fitness.numIterations;
        while(repeat--){
            
            for(size_t i=cutoff; i < population.size(); ++i){ // iterate all the bad constants in the population
                int a = Random::randomInt( cutoff ), b = Random::randomInt(cutoff - 1); // pick 2 random good score roots from the best of the population
                if(b >= a) ++b; // give all good score nodes a chance at selection

                // Copy data from one of the better nodes and cross mutate with another better node
                mutateCross(population[i], population[a], population[b]);
                // Mutate the data slightly by changing its value by up to half itself
                mutateChange(population[i]);
                // Calculate the RMS score for the node
                updateScore(population[i]);
            }
            
            sortPopulation(); // sort the population
        }
    }

    syncConstants(population[0], true); // sync best population to original root
    return population[0].score; // return the highest score
}
