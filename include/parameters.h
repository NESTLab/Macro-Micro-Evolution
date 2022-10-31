/**			Class Parameters: Holds Parameter Data For The Node System
 * 	Usage:
 * 		Parameters:::Init()			 Initialize the parameters resources
 *		Parameters::Load(<filepath>) Load the parameters from a config file
 * 		Parameters::Params()		 Get the current parameters instance pointer
 * 		Parameters::Free()			 Make sure to free the parameters resources when done
 * **/

#ifndef __PARAMS_H_
#define __PARAMS_H_

class Parameters; // pre-declare

#include "forward.h"
#include "visualevo.h"
#include "operators.h"
#include "nodetypes.h"
#include "stringparser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <string>
#include <cmath>
#include <type_traits>

struct ComplexityOp {
	uint8_t ls, rs;
	float complexity;
};

struct FitnessParameters {
	bool use;
	uint32_t popSize, numIterations;
	double changeChance, sampleSize, cutOff;
};

struct VisualParameters {
	bool display, closeOnFinish;
	uint32_t clearCount, xresolution, yresolution;
	float xscale, yscale;
	std::weak_ptr<VisualEvo::Graph> graph;
	uint32_t drawVarIndex;
};

typedef std::vector<ComplexityOp> ComplexityList;
typedef std::vector<ComplexityList> ComplexityMap;
typedef std::vector<std::string> StringMap;

class Parameters {
private:
	Parameters();
	static Parameters* globalParams;

	void processParameters();
public:
	static inline Parameters* Params() { return globalParams; } const
	static inline void Init() { globalParams = new Parameters; StringParser::params = globalParams; }
	static inline void Free() { delete globalParams; globalParams = nullptr; }
	static void Load(const std::string& path);


	enum OperandType {
		NONE,
		CONSTANT,
		OPERATOR
	};
	// Internal Parameters - Converted Parameters
	int decimalPlacesExp;
	Operators::FunctionList operatorList;

	// External Parameters
	size_t popSize, generationCount, maxScoreHistory, mutationCount, targetComplexity, maxDuplicateRemoval, popSave;
    int decimalPlaces;
	VTYPE maxConstant, minConstant, minRMSClamp, maxRMSClamp;
	double defaultComplexity, survivalRatio, weightChance,
		   constantChance, operatorChance, changeChance, mutationChance,
		   parsimony, accuracy;
	bool singleThreaded, weighedMutation, verboseLogging, useSqrtRMS, useRMSClamp, useCCMScoring, useVariableDescriptors;
	
	std::string precalculatedTree, defaultPointCloudCSV;

	FitnessParameters fitness;
	VisualParameters visual;

	std::vector<NodeTypes::FunctionName> operatorFunctions;
	std::map<int, RootNode*> variableDescriptors;
	int denySimplifyOperator;

	Operators::EqPoints points; // may not be used internally

	ComplexityMap complexity;
	StringMap signs;

	virtual ~Parameters();
};

#endif
