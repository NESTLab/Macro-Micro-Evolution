//Include JSON Parse Library
#include "jsonloader.h"
#include "parameters.h" // note: visualevo includes X11 for linux which has name collisions with RapidJson - must include AFTER jsonloader

#include "evorootnode.h"

using namespace Operators;
using namespace NodeTypes;

Parameters* Parameters::globalParams = nullptr;

Parameters::Parameters() {
    if(globalParams != nullptr) throw std::runtime_error("Cannot have more than one instance of parameters");
    
    /**-------------------------------------
     * Core Internal Parameters DO NOT MODIFY
     * -------------------------------------**/
    {
        size_t tyCount = FunctionNameString.size(); // number of node types
        complexity.resize(tyCount , {}); // initialize complexity map
        operatorList.resize(tyCount, {nullptr, 0}); // initialize operator list
        //signs.resize(tyCount, ""); // initialize strings of signs

        // Function Pointers To Core Operators
        operatorList[INVERSE] = Operator {&Inverse, 1};
        operatorList[NEGATIVE] =Operator {&Negative, 1};
        operatorList[ADD] =     Operator {&Add, 2};
        operatorList[SUBTRACT] =Operator {&Subtract, 2};
        operatorList[MULTIPLY] =Operator {&Multiply, 2};
        operatorList[DIVIDE] =  Operator {&Divide, 2};
        operatorList[POWER] =   Operator {&Power, 2};
        operatorList[ABS] =     Operator {&Abs, 1};
        operatorList[SIN] =     Operator {&Sin, 1};
        operatorList[COS] =     Operator {&Cos, 1};
        operatorList[TAN] =     Operator {&Tan, 1};

        /* Strings Of Signs/Operators - NOT USED
        signs[INVERSE] = "1./";
        signs[NEGATIVE] = "-";
        signs[ADD] = "+";
        signs[SUBTRACT] = "-";
        signs[MULTIPLY] = "*";
        signs[DIVIDE] = "/";
        signs[POWER] = "**";
        signs[ABS] = "|";
        signs[SIN] = "(sin))";
        signs[COS] = "(cos)";
        signs[TAN] = "(tan)";
        */
    }

    /*
    *   -----------------------------------------------
    *
    *   External Parameters To Be Loaded By config.json
    *
    *   -----------------------------------------------
    * */

    // Default Parameters (No config.json loaded)

    popSize = 4000;         // preset population size for each generation
    generationCount = 75;   // max number of generations to iterate
    survivalRatio = 0.05;   // ratio of nodes to keep after each generation
    targetComplexity = 10;  // weigh the score toward this complexity value
    parsimony = 0.65;       // how important is the accuracy relative to complexity (<percent> focus on accuracy)
    maxScoreHistory = 5;    // history of best population scores to keep
    accuracy = 0.02;        // stop when the score reaches below this threshold
    useCCMScoring = false;  // uses cross correlation macro scoring instead of RMS scoring (only used for non-fitness scoring) (This feature is currently not implemented)
    weighedMutation = true; // cross copy mutation will pull nodes weighed toward the beginning of the population
    weightChance = 1.5;     // percent chance that the node is picked
    maxDuplicateRemoval = 0;// maximum retries for duplicate node tree removal
    popSave = 0;            // number of root nodes to save an original copy of when running the fitness function

    mutationCount = 2;      // the number of mutation iterations to process on a new child during repopulation
    decimalPlaces = 2;      // the maximum number of decimal places a value will end up with
    maxConstant = 100;      // largest number before becoming infinity
    minConstant = 0.1;      // smallest number before becoming 0
    minRMSClamp = -500;     // minimum value allowed for results and point cloud data during RMS scoring
    maxRMSClamp = 500;      // maximum value allowed for results and point cloud data during RMS scoring
    useRMSClamp = true;     // enable the RMS score result and point cloud data value clamping
    constantChance = 50;    // chance that a varnode becomes a constant vs a variable
    changeChance = 60;      // chance that a mutation will be a change mutation
    operatorChance = 50;    // chance that a change mutation will change the selected node to an opnode vs a varnode
    mutationChance = 50;    // chance that a mutation will occur during repopulation
    useSqrtRMS = true;      // additionally use sqrt when calculating RMS - turning this off might provide slightly better performance
    points.numVars = 1;     // the number of variables used in the given equation
    

    defaultPointCloudCSV = "default.csv"; // default CSV file to load for point cloud data if no file is specified
    precalculatedTree = ""; // this string may contain a precalculated tree structure - this structure will be the first root node in the beginning generation
                            // the syntax for this string is the same for calling the string() method on any node
    verboseLogging = true;  // use a verbose logging for debugging purposes
    singleThreaded = false; // use only one single thread for processing multi-threaded tasks - this is only for debugging purposes and will greatly affect performance - you've been warned

    // Default Fitness Function Parameters

    fitness.use = true;         // the whole point to this algorithm - if you turn this off then there will be no sub-algorithm
    fitness.popSize = 50;       // population size of constants
    fitness.changeChance = 75;  // chance that a constant can change during mutation
    fitness.sampleSize = 0.5;   // sample size ratio of actual data that is loaded
    fitness.cutOff = 0.1;       // population cutoff ratio for the constants
    fitness.numIterations = 10; // number of generations the fitness will cycle (mutate, score, and sort)
    
    // Default Visual Evo Parameters
    visual.display = true;      // display the VisualEvo window
    visual.closeOnFinish = true;// close the VisualEvo window when program finishes - otherwise program will stay running until user closes window
    visual.clearCount = 5;      // clear the graph after this nuimber of draws
    visual.xresolution = 1024;  // x-axis resolution for the graph surface
    visual.yresolution = 1024;  // y-axis resolution for the graph surface
    visual.xscale = 8;          // x-scale for the graph when drawing lines/points
    visual.yscale = 8;          // y-scale for the graph when drawing lines/points
    visual.drawVarIndex = 0;    // when multi-variable point cloud data is given, choose to draw a specific variable index

    useVariableDescriptors = false; // variable descriptors are used to compute a more realistic complexity when two or more variables are shared between common equations

    // List of operators to choose from randomly
    // - Enable or Disable Operators Here - See nodetypes.h for list of existing operators and function names - DO NOT ADD NON-OPERATORS
    operatorFunctions = {
        INVERSE, NEGATIVE,
        ADD, SUBTRACT,
        MULTIPLY, DIVIDE, POWER,
        //ABS, SIN, COS, TAN
    };

    denySimplifyOperator = -1; // prevents the operator from being simplified (-1 is default and means this feature is disabled)
    
    //Complexity For Operator / Variables Confiugrations

    defaultComplexity = 0;

    complexity[INVERSE] = {
        {CONSTANT, NONE , 1}, { OPERATOR, NONE , 1}
    };
    complexity[NEGATIVE] = {
        {CONSTANT, NONE , 1}, { OPERATOR, NONE , 1}
    };
    complexity[ADD] = {
        {OPERATOR, CONSTANT , 2}, {OPERATOR, OPERATOR , 2},
        {CONSTANT, OPERATOR , 2}, {CONSTANT, CONSTANT , 2}
    };
    complexity[SUBTRACT] = {
        {OPERATOR, CONSTANT , 2}, {OPERATOR, OPERATOR , 2},
        {CONSTANT, OPERATOR , 2}, {CONSTANT, CONSTANT , 2}
    };
    complexity[MULTIPLY] = {
        {OPERATOR, CONSTANT , 2}, {OPERATOR, OPERATOR , 2},
        {CONSTANT, OPERATOR , 2}, {CONSTANT, CONSTANT , 2}
    };
    complexity[DIVIDE] = {
        {OPERATOR, CONSTANT , 2}, {OPERATOR, OPERATOR , 2},
        {CONSTANT, OPERATOR , 2}, {CONSTANT, CONSTANT , 2}
    };
    complexity[POWER] = {
        {OPERATOR, CONSTANT , 2}, {OPERATOR, OPERATOR , 4},
        {CONSTANT, OPERATOR , 4}, {CONSTANT, CONSTANT , 2}
    };
    complexity[ABS] = {
        {CONSTANT, NONE , 1}, { OPERATOR, NONE , 1}
    };
    complexity[SIN] = {
        {CONSTANT, NONE , 2}, { OPERATOR, NONE , 4}
    };
    complexity[COS] = {
        {CONSTANT, NONE , 2}, { OPERATOR, NONE , 4}
    };
    complexity[TAN] = {
        {CONSTANT, NONE , 2}, { OPERATOR, NONE , 4}
    };    

    //-----------------------------------------------------------------------
    
    processParameters(); // post-parameter initialization configuration - don't touch
}


/*
    Process External Parameters Into
        Internal Parameters Used By The EvoAlgo System
*/
void Parameters::processParameters() {
    decimalPlacesExp = std::pow(10, decimalPlaces); // pre-calculate decimal exponent for multiplication
}

void Parameters::Load(const std::string& path) {
    {
        std::stringstream cache;
        std::ifstream file(path);
        if(file.is_open()){
            cache << file.rdbuf();
            file.close();
        }
        if(!json::parseData(cache.str().data(), cache.str().size())){
            warning("Invalid \"config.json\" file");
            return;
        }
    }

    json::Object& config = json::_jsonData;

    {
        json::loadProperty("populationSize", globalParams->popSize);
        json::loadProperty("defaultComplexity", globalParams->defaultComplexity);
        json::loadProperty("generationCount", globalParams->generationCount);
        json::loadProperty("survivalRatio", globalParams->survivalRatio);
        json::loadProperty("targetComplexity", globalParams->targetComplexity);
        json::loadProperty("parsimonyRatio", globalParams->parsimony);
        json::loadProperty("accuracyCompletion", globalParams->accuracy);
        json::loadProperty("useCCMScoring", globalParams->useCCMScoring);
        json::loadProperty("weighedMutation", globalParams->weighedMutation);
        json::loadProperty("weightChance", globalParams->weightChance);
        json::loadProperty("maxDuplicateRemoval", globalParams->maxDuplicateRemoval);
        json::loadProperty("populationCopyCount", globalParams->popSave);
        
        json::loadProperty("mutationCount", globalParams->mutationCount);
        json::loadProperty("decimalPrecision", globalParams->decimalPlaces);
        json::loadProperty("minConstant", globalParams->minConstant);
        json::loadProperty("maxConstant", globalParams->maxConstant);
        json::loadProperty("minRMSClamp", globalParams->minRMSClamp);
        json::loadProperty("maxRMSClamp", globalParams->maxRMSClamp);
        json::loadProperty("useRMSClamp", globalParams->useRMSClamp);
        json::loadProperty("constantChance", globalParams->constantChance);
        json::loadProperty("changeChance", globalParams->changeChance);
        json::loadProperty("operatorChance", globalParams->operatorChance);
        json::loadProperty("mutationChance", globalParams->mutationChance);
        json::loadProperty("useSqrtRMS", globalParams->useSqrtRMS);
        json::loadProperty("defaultCSV", globalParams->defaultPointCloudCSV);
        json::loadProperty("precalculatedTree", globalParams->precalculatedTree);
        json::loadProperty("verboseLogging", globalParams->verboseLogging);
        json::loadProperty("singleThreaded", globalParams->singleThreaded);
    }

    if(config.HasMember("fitnessAlgo") && config["fitnessAlgo"].IsObject()){
        rapidjson::Value& cfg = config["fitnessAlgo"];
        json::loadProperty(cfg, "enabled", globalParams->fitness.use);
        json::loadProperty(cfg, "sampleRatio", globalParams->fitness.sampleSize);
        json::loadProperty(cfg, "populationSize", globalParams->fitness.popSize);
        json::loadProperty(cfg, "iterationCount", globalParams->fitness.numIterations);
        json::loadProperty(cfg, "survivalRatio", globalParams->fitness.cutOff);
        json::loadProperty(cfg, "changeChance", globalParams->fitness.changeChance);
    }
    
    if(config.HasMember("visualEvo") && config["visualEvo"].IsObject()){
        rapidjson::Value& cfg = config["visualEvo"];
        json::loadProperty(cfg, "enabled", globalParams->visual.display);
        json::loadProperty(cfg, "closeOnFinish", globalParams->visual.closeOnFinish);
        json::loadProperty(cfg, "clearGraphCount", globalParams->visual.clearCount);
        json::loadProperty(cfg, "xresolution", globalParams->visual.xresolution);
        json::loadProperty(cfg, "yresolution", globalParams->visual.yresolution);
        json::loadProperty(cfg, "xscale", globalParams->visual.xscale);
        json::loadProperty(cfg, "yscale", globalParams->visual.yscale);
        json::loadProperty(cfg, "drawVariableIndex", globalParams->visual.drawVarIndex);
    }

    json::loadProperty("useVariableDescriptors", globalParams->useVariableDescriptors);

    // load variable descriptors
    if(config.HasMember("variableDescriptors")){
        rapidjson::Value& cfg = config["variableDescriptors"];
        if(cfg.IsArray()){
            globalParams->variableDescriptors.clear(); // clear defaults
            auto ar = cfg.GetArray();
            int varNum = -1; // variable number - each array index is associated with the variable in the cloud-point data
            for(rapidjson::Value::ConstValueIterator itr = ar.Begin(); itr != ar.End(); ++itr){
                if(itr->IsString()){
                    std::string descriptor = itr->GetString();
                    if(descriptor.empty()) continue;
                    
                    if(RootNode::params == nullptr) RootNode::params = globalParams;

                    RootNode* rt = new RootNode;
                    if(!rt->parseRootNodeString(descriptor)){
                        delete rt;
                        warning("bad variable descriptor found when parsing: \"" + descriptor + "\" is not valid");
                        continue;
                    }
                    globalParams->variableDescriptors.insert_or_assign(++varNum, rt); // update variable descriptor for iterated variable number
                }
            }

        } else {
            warning("invalid variableDescriptors list");
        }
    }

    // load allowed operator functions list
    if(config.HasMember("operatorFunctions")){
        rapidjson::Value& cfg = config["operatorFunctions"];
        if(cfg.IsArray()){
            globalParams->operatorFunctions.clear(); // clear defaults
            auto ar = cfg.GetArray();
            for(rapidjson::Value::ConstValueIterator itr = ar.Begin(); itr != ar.End(); ++itr){
                if(itr->IsString()){
                    std::string str = itr->GetString();
                    auto e = std::find(NodeTypes::FunctionNameString.begin(), NodeTypes::FunctionNameString.end(), str);
                    size_t pos = e - NodeTypes::FunctionNameString.begin();
                    if(e == NodeTypes::FunctionNameString.end() || pos < 5){ // position correlates to operator name
                        warning("operatorFunctions has invalid operator: " + str);
                    } else {
                        globalParams->operatorFunctions.push_back(NodeTypes::FunctionName(pos));
                    }
                }
            }

        } else {
            warning("invalid operatorFunctions list");
        }
    }

    std::string denyOp;
    if(json::loadProperty("denySimplifyOperator", denyOp) && denyOp != ""){
        auto e = std::find(NodeTypes::FunctionNameString.begin(), NodeTypes::FunctionNameString.end(), denyOp);
        size_t pos = e - NodeTypes::FunctionNameString.begin();
        if(e == NodeTypes::FunctionNameString.end() || pos < 5){ // position correlates to operator name
            warning("denySimplifyOperator has invalid operator: " + denyOp);
        } else {
            globalParams->denySimplifyOperator = NodeTypes::FunctionName(pos);
        }
    }

    // load complexity operator values
    if(config.HasMember("complexityWeights")){
        
        // quickly map operator complexity list types into a string format for quick lookup
        const std::map<std::string, OperandType> OpTypes = { 
            std::pair("constant", OperandType::CONSTANT),
            std::pair("operator", OperandType::OPERATOR),
            std::pair("", OperandType::NONE)
        };
        rapidjson::Value& cfg = config["complexityWeights"];
        if(cfg.IsObject()){

            for(rapidjson::Value::ConstMemberIterator itr = cfg.MemberBegin(); itr != cfg.MemberEnd(); ++itr){
                if(itr->name.IsString()){
                    std::string str = itr->name.GetString();
                    auto e = std::find(NodeTypes::FunctionNameString.begin(), NodeTypes::FunctionNameString.end(), str);
                    size_t pos = e - NodeTypes::FunctionNameString.begin();
                    if(e == NodeTypes::FunctionNameString.end() || pos < 5){ // position correlates to operator name :: non-ops are less than 5
                        warning("complexityWeights has invalid operator: " + str);
                    } else { // valid operator - check weights
                        const rapidjson::Value& list = itr->value; // get external complexity list for operator
                        if(!list.IsArray()){
                            warning("complexityWeights has invalid operator list for operator: " + str);
                            continue;
                        }
                        NodeTypes::FunctionName oper = NodeTypes::FunctionName(pos);
                        ComplexityList& clist = globalParams->complexity[oper]; // get internal complexity list for operator
                        if(clist.size() != list.Size()){
                            warning("complexityWeights has wrong number of weights for operator: " + str + " ; requires " + std::to_string(clist.size()) + " but " + std::to_string(list.Size()) + " given.");
                            continue;
                        }
                        ComplexityList cacheComplexity;
                        for(rapidjson::Value::ConstValueIterator _itr = list.Begin(); _itr != list.End(); ++_itr){
                            if(!_itr->IsArray()) break; // error will show because the cache size will mismatch
                            
                            std::string lhs, rhs;
                            ComplexityOp mcwo {};

                            // {lhs, rhs, price}
                            // {"const/op", "const/op/ ", price}
                            const auto& complexities = _itr->GetArray();
                            if( !complexities[0].IsString() ||
                                !complexities[1].IsString() ||
                                !complexities[2].IsNumber()){
                                syslog::cout << "list fails to match strict types: {string, string, number}\n";
                                break; // fail strict list check
                            }
                            lhs = complexities[0].GetString();
                            rhs = complexities[1].GetString();
                            mcwo.complexity =
                                complexities[2].IsLosslessFloat() ? complexities[2].GetFloat() :
                                complexities[2].IsLosslessDouble() ? static_cast<float>(complexities[2].GetDouble()) :
                                static_cast<float>(complexities[2].GetInt64());

                            if(!OpTypes.count(lhs) || !OpTypes.count(rhs)){
                                syslog::cout << "failed to find " << lhs << " OR " << rhs << " as a valid OpType\n";
                                break; // fail lookup
                            }
                            // finish constructing mapped complexity weight operation
                            mcwo.ls = static_cast<uint8_t>(OpTypes.at(lhs));
                            mcwo.rs = static_cast<uint8_t>(OpTypes.at(rhs));

                            cacheComplexity.emplace_back(std::move(mcwo));
                        }
                        if(cacheComplexity.size() != list.Size()){
                            warning("complexityWeights has one or more invalid weights for operator: " + str);
                            continue;
                        }
                        clist.clear(); // clear the complexity-weight operator list to begin populating with new data
                        clist = std::move(cacheComplexity); // move cached complexities into official mapped complexity weight table
                    }
                }
            }

        } else {
            warning("invalid complexityWeights object");
        }
    }

    globalParams->processParameters(); // post-parameter initialization configuration - don't touch
}


Parameters::~Parameters() {
    for(auto& p : variableDescriptors){ // free memory of all variable descriptor RootNodes
        delete p.second;
    }
}