#include "evoalgo.h"

using namespace NodeTypes;

EvoAlgo::EvoAlgo(const Parameters* params, const Operators::EqPoints& data): params(params), data(data), generation(0), drawGraphCount(0) {
    if(!params->fitness.use) warning("Fitness algorithm is turned off. User doesn't have brain.");
    if(params->singleThreaded) warning("Notice: User has enabled the single threaded feature - multi-threaded tasks will no longer run on more than 1 thread!");
    if(params->useCCMScoring) warning("useCCMScoring was enabled but this feature is currently not implemented yet");

    graph = params->visual.graph; // get visual graph access

    scoreDatabase.resize(params->maxScoreHistory, INFINITY);

    population.resize(params->popSize, nullptr); // construct a new population

    if(params->useVariableDescriptors){
        shadowPopulation.resize(params->popSize, nullptr); // construct a new shadow population
    }

    float sz = double(calculatePoolSize()) * (double(population.size()) + double(shadowPopulation.size())) / 1024.f / 1024.f;

    debug("pre-allocating population node pools: this will take approximately " + std::to_string(sz) + "MB of system memory", true);
    threadGenerator(0, population.size(), &workRootNodeAllocator);

    /*
    for(size_t i=0; i < population.size(); ++i){
        RootNode*& rt = population[i];
        rt = new RootNode(params);
        // first node loaded from parameters
        if(i != 0 || !parseRootNodeString(params->precalculatedTree, *rt)){
            rt->node = (Node*) rt->createNode(RANDOM_OP, true);
            rt->node = rt->mutateAdd(3); // add 3 mutations to all nodes in the population
        }
        rt->score = rt->node->score(data);
    }
    */

    sortPopulation();

    for(int i=0;i < std::min(4, int(population.size())); ++i){
        drawGraph(*population[i]); // immediately draw the 5 best in the pre-generated population
    }
}

EvoAlgo::~EvoAlgo() {
    for(RootNode* rt : population){
        delete rt;
    }
    for(RootNode* rt : shadowPopulation){
        delete rt;
    }
}

// Mutate the passed root node randomly
void EvoAlgo::mutate(RootNode& rt, int iters) {
    if(Random::chance(params->changeChance)){ // check if the node should process a change mutation
        rt.node = rt.mutateChange(iters); // process a change mutation
    } else {
        rt.node = Random::chance(50) ? // randomly add or remove mutation
            rt.mutateAdd(iters) :
            rt.mutateRemove(iters);
    }
}

void EvoAlgo::repopulate() {
    uint32_t cutoff = std::round(params->popSize * params->survivalRatio),
             dead = population.size() - cutoff;
    
    debug(std::string("cutoff: ") + std::to_string(cutoff) + " dead: " + std::to_string(dead));
    
    // Cross mutation + node mutation iteration - generate new mutated children

    for(size_t i=cutoff;i < population.size(); ++i){
        population[i]->complete = false; // reset complete status for thread workers
    }
    threadGenerator(cutoff, population.size(), &workRepopulate, &cutoff); // generate workers for the repopulation - give the cutoff region

    debug("additional mutations for duplicates");
    size_t count = 0, retry = 0; // keep track of total number of re-mutations
    do {
        size_t unique = 0;
        for(size_t i=cutoff; i < population.size(); ++i){ // find duplicate forms in newly populated area
            RootNode& rt = *population[i];
            // search for another node with the same form
            auto itt = std::find_if(population.begin() + cutoff + i + 1, population.end(), [&](const RootNode* ort){
                    return (ort->form == rt.form);
                });
            if( itt == population.end() ) { // did not find a match
                ++unique;
            } else { // found a duplicate - re-mutate then recalcualte
                mutate(rt, 3); // mutate the duplicate nodes to remove duplicates
                rt.calculateForm();
            }
        };

        if(unique < dead){
            count += (dead - unique);
        } else break; // no more duplicates found - break from duplicate finder

        if(++retry == params->maxDuplicateRemoval) break; // break if reached max retries
    } while(1); // must check for duplicates until no more are found

    if(count){
        debug("finished accumulated re-mutation of " + std::to_string(count) + " root nodes");
    }
}

void EvoAlgo::sortPopulation() {
    std::sort(population.begin(), population.end(), [](RootNode* l, RootNode* r) {
        return (l->score < r->score); // sort population with best scores first to last
    });
}

// Threaded Workers:

void EvoAlgo::workRootNodeAllocator(EvoAlgo* _this, size_t i, size_t end, size_t spread, void* extra) { // allocate root nodes simultaniously
    if(i >= end) return; // pre-check
    do {
        RootNode*& rt = _this->population[i];

        rt = new RootNode(_this->params);
        // first node loaded from parameters
        if(i != 0 || !rt->parseRootNodeString(_this->params->precalculatedTree)){
            rt->node = (Node*) rt->createNode(RANDOM_OP, true);
            rt->node = rt->mutateAdd(3); // add 3 mutations to all nodes in the population
        }
        rt->score = rt->node->score(_this->data);

        // Shadow population root node allocation
        if(_this->params->useVariableDescriptors){
            RootNode*& shdrt = _this->shadowPopulation[i];

            shdrt = new RootNode(_this->params);
        }
        
    } while((i += spread) < end);
}

void EvoAlgo::workSimplifyScoreComplexity(EvoAlgo* _this, size_t i, size_t end, size_t spread, void* extra) { // score population worker
    if(i >= end) return; // pre-check
    do {
        RootNode& rt = *_this->population[i];

        if(!rt.complete){
            /// --------------------------- Iteration
            rt.simplify(); // simplify root node

            double calculatedComplexity = __DBL_MAX__; // this is the precalculated complexity - initialize to max value - always set to the lowest existing value

            if(_this->params->useVariableDescriptors){ // uses variable descriptors for calculating complexity
                RootNode& shdrt = *_this->shadowPopulation[i];
                if(shdrt.node != nullptr) shdrt.node->freeAll();

                shdrt.node = rt.node->copy(&shdrt); // copy current root node to shadow root node for alternate complexity parsing

                for(int var = 0; var < _this->data.numVars; ++var){
                    if(!_this->params->variableDescriptors.count(var)) continue; // no variable descriptor found
                    shdrt.node = shdrt.findReplace(var, _this->params->variableDescriptors.at(var)->node);
                }

                shdrt.simplify(); // re-simplify the shadow root node -             Kodi shoutout *Woof!*

                calculatedComplexity = std::fmin(calculatedComplexity, shdrt.node->computeComplexity());
            }

            calculatedComplexity = std::fmin(calculatedComplexity, rt.node->computeComplexity());

            rt.complexity = calculatedComplexity; // update the complexity of the rootnode to the new calculated complexity
            rt.score = rt.node->score(_this->data);

            /// --------------------------- End Iteration
            rt.complete = true;
        } else {
            warning("Danger: A thread skipped work to do because the node was not ready -> workSimplifyScoreComplexity");
        }

    } while((i += spread) < end);
}

void EvoAlgo::workScore(EvoAlgo* _this, size_t i, size_t end, size_t spread, void* extra) { // score population worker
    if(i >= end) return; // pre-check
    do {
        RootNode& rt = *_this->population[i];

        if(!rt.complete){
            /// --------------------------- Iteration
            rt.score = rt.node->score(_this->data);
            /// --------------------------- End Iteration
            rt.complete = true;
        } else {
            warning("Danger: A thread skipped work to do because the node was not ready -> workScore");
        }

    } while((i += spread) < end);
}

void EvoAlgo::workFitness(EvoAlgo* _this, size_t i, size_t end, size_t spread, void* extra) { // fitness iterator for a worker
    if(i >= end) return; // pre-check
    do {
        RootNode& rt = *_this->population[i];

        if(!rt.complete){
            /// --------------------------- Iteration
            rt.score = rt.node->score(_this->data, true); // use fitness evolution
            /// --------------------------- End Iteration
            rt.complete = true;
        } else {
            warning("Danger: A thread skipped work to do because the node was not ready -> workFitness");
        }

    } while((i += spread) < end);
}

void EvoAlgo::workRepopulate(EvoAlgo* _this, size_t i, size_t end, size_t spread, void* extra) {
    if(i >= end) return; // pre-check
    uint32_t cutoff = *(uint32_t*)extra; // get cutoff location

    do {
        RootNode& rt = *_this->population[i];

        if(!rt.complete){
            /// --------------------------- Iteration
            
            size_t origEq = 0, copyEq = 0; // index in population for the root nodes to copy from
            
            rt.node->freeAll(); // free node from memory
            rt.node = nullptr; // prevent bad pointer
            rt.score = INFINITY; // reset score

            // pick a random index for the cross mutation
            if(_this->params->weighedMutation){ // check if pick should be weighed toward lower scores from the previous generation
                for(size_t k=0;k<cutoff; ++k)
                    if(Random::chance(_this->params->weightChance)) {origEq = k; break;}
                for(size_t k=0;k<cutoff; ++k)
                    if(Random::chance(_this->params->weightChance)) {copyEq = k; break;}
            } else {
                origEq = Random::randomInt(cutoff-1);
                copyEq = Random::randomInt(cutoff-1);
            }
            
            // setup original node and copy node roots for cross mutation
            const Node* origNode = _this->population[origEq]->node, // the original root to copy
                      * copyNode = _this->population[copyEq]->node; // the destination rot node for mutation
            
            // get list of all sub nodes in original node - also get list of nodes from root destination node
            NodeList nls, ccl;

            origNode->listOfNodes(nls);
            copyNode->listOfNodes(ccl);
            rt.node = origNode->copyMutate(&rt, // new root node to copy to
                                           nls.all[Random::randomInt(nls.all.size() - 1)],
                                           ccl.all[Random::randomInt(ccl.all.size() - 1)]); // crossover copy and generate new node
            
            rt.node->setParent(nullptr); // make sure the new root node doesn't have a parent!

            if(Random::chance(_this->params->mutationChance)){ // the chance the new node will be mutated
                _this->mutate(rt, _this->params->mutationCount); // mutate new child node
            }

            rt.calculateForm(); // pre-calculate the new form

            /// --------------------------- End Iteration
            rt.complete = true;
        } else {
            warning("Danger: A thread skipped work to do because the node was not ready -> workRepopulate");
        }

    } while((i += spread) < end);


}


void EvoAlgo::threadGenerator(size_t start, size_t stop, Worker worker, void* extra) { // create workers to iterate over the list
    const volatile size_t hardwarethreads = std::thread::hardware_concurrency(); // must be volatile because we are changing a constant! *queue Wow! sequence*
    if(params->singleThreaded){
        const_cast<size_t&>(hardwarethreads) = 1; // use one thread for debugging if user enabled single threaded
    }

    std::vector<std::thread> threads;

    debug("begin a threaded task");
    // Thread this iteration
    for(size_t i=0; i < hardwarethreads; ++i){
        threads.emplace_back(std::thread(worker, this, start + i, stop, hardwarethreads, extra));
    }

    for(std::thread& t : threads) t.join();
    debug("finished a threaded task");
}

// --------------------------

void EvoAlgo::drawGraph(RootNode& rt) {
    if(!graph.expired()){ // check if graph exists
        std::shared_ptr<VisualEvo::Graph> graph = this->graph.lock();
        if(params->points.points.size()){ // check if there are actually any points to calculate
            if(!drawGraphCount++){
                graph->drawClear(); // clear graph after clear count reached
                graph->drawAdd(params->points, 0x3209FF10, VisualEvo::Graph::BUBBLES); // original point cloud data
                debug("Refresh Graph -> " + std::to_string(params->points.points.size()) + " points of data");
            }

            Operators::EqPoints data;   // calculate best rootnode tree here
            rt.computeEquation(data, params->points.points.front()[0], // from lowest x value
                                    params->points.points.back()[0]); // to highest x value

            float dist = float(drawGraphCount) / float(params->visual.clearCount);
            graph->drawAdd(data, olc::Pixel(255, dist * 255, 10 + dist*100) ); // calculated
        }
        // graph->draw();
        if(drawGraphCount == params->visual.clearCount) drawGraphCount = 0; // reset counter
    }
}

bool EvoAlgo::checkForMemoryConsistency() {
    for(RootNode* rt : population){
        int diff = rt->validateNodeTree();
        if(diff != 0) {
            warning("Found a broken node tree with " + std::to_string(diff) + " leaked nodes");
            return false;
        }
    }
    for(RootNode* rt : shadowPopulation){
        int diff = rt->validateNodeTree();
        if(diff != 0) {
            warning("Found a broken node tree with " + std::to_string(diff) + " leaked nodes");
            return false;
        }
    }
    return true;
};

bool EvoAlgo::iteration() {
    Clock timer, genTimer;

    generation++;
    syslog::cout << "\n----------------------\nstarting generation " << generation << " / " << params->generationCount << "\n----------------------\n";


    debug(std::string("Node Count: ") + std::to_string(NodePool::getTotalNodeCount()));
    
    {
        size_t lgst = 0;
        for(RootNode* rt : population) lgst = std::max(lgst, rt->pool.getNodeCount());
        std::string msg = std::string("Largest Node Cluster: ") + std::to_string(lgst);
        if(lgst < 80) debug(msg); else warning(msg);
    }

    // generate new population
    timer.restart();
    debug("repopulate()");
    repopulate();
    debug(timer.getMilliseconds());

    // update score
    timer.restart();
    debug("update score");
    for(size_t i=0;i < population.size(); ++i){
        population[i]->complete = false; // reset complete status for thread workers
    }
    threadGenerator(0, population.size(), &workScore); // threaded scoring
    debug(timer.getMilliseconds());

    // sort new scored population
    timer.restart();
    debug("sortPopulation()");
    sortPopulation();
    debug(timer.getMilliseconds());

    // fitness cutoff for surviving population
    timer.restart();
    debug("fitness scoring");
    if(params->fitness.use){ // thread fitness iterator

        size_t bestLength = (params->popSize * params->survivalRatio);

        for(size_t i=0;i < bestLength; ++i){
            population[i]->complete = false; // reset complete status for thread workers
        }

        if(params->popSave >= population.size() - bestLength){
            warning("populationCount is larger than the surviving population size. This causes an overflow to leak into the surviving population which could cause a crash.");
        }

        for(size_t i = 0; i < params->popSave; ++i){
            RootNode* rt = population[i + population.size() - params->popSave]; // the rootnode pool we are updating
            Node*& root = rt->node; // root node to overwrite
            root->freeAll(); // sorry but kill these last rootnodes to make room for the good copies
            root = population[i]->node->copy(rt); // update the root to a copy of the best node + popSave offset
        }
        threadGenerator(0, bestLength, &workFitness); // generate threads
    }
    debug(timer.getMilliseconds());

    // simplify + score
    timer.restart();
    debug("simplification and re-score");

    for(size_t i=0;i < population.size(); ++i){
        population[i]->complete = false; // reset complete status for thread workers
    }
    threadGenerator(0, population.size(), &workSimplifyScoreComplexity); // generate threads to simplify and solve the complexity
    debug(timer.getMilliseconds());

    // sort population after scoring for targeted complexities
    timer.restart();
    debug("sortPopulation()");
    sortPopulation();
    debug(timer.getMilliseconds());

    // update score based on user-defined parsimony and the target complexity
    timer.restart();
    debug("complexity and parsimony scoring");
    float minScore = population[std::floor(params->survivalRatio * population.size())]->score;
    double a = params->parsimony, b = 1 - a;

    for(RootNode*& rt : population){
        float acWeight = rt->score / minScore,
              cxWeight = std::max(0., double(rt->complexity - params->targetComplexity) / params->targetComplexity);
        rt->score = a * acWeight + b * cxWeight;
    }
    debug(timer.getMilliseconds());
    
    // sort population after scoring for targeted complexities
    timer.restart();
    debug("sortPopulation()");
    sortPopulation();
    debug(timer.getMilliseconds());

    // update best scores to the score database
    scoreDatabase.insert(scoreDatabase.begin(), population[0]->score);
    scoreDatabase.pop_back(); // remove oldest score

    RootNode& best = *population[0];
    float ac_score = best.node->score(data); // calculate one more time for accuracy check

    drawGraph(*population[0]); // draw again on graph

    if(Node::debugSkippedSimplify > 0){
        debug(std::string("Skipped ") + NodeTypes::FunctionNameString[params->denySimplifyOperator] + " simplify " + std::to_string(Node::debugSkippedSimplify) + " times");
        Node::debugSkippedSimplify = 0; // reset debugging counter
    }

    checkForMemoryConsistency();

    bool complete = ac_score <= params->accuracy || generation >= params->generationCount;

    syslog::cout << (complete ? "Final ":"") << "Best GenPop: " << best.node->string() <<
                 "\n      " << (complete ? "Final ":"") << "Score: " << best.score <<
                 "\n   " << (complete ? "Final ":"") << "Accuracy: " << ac_score <<
                 "\n " << (complete ? "Final ":"") << "Complexity: " << best.complexity << "\n";

    
    debug(std::string("generation elapsed time: ") + std::to_string(genTimer.getMilliseconds()) + "ms", true);

    return (ac_score <= params->accuracy);

}

void EvoAlgo::run() {

    while(generation < params->generationCount) {
        if(iteration()) break;
    }


    syslog::cout << "---------- Finished ----------" << "\n";
}