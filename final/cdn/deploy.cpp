#include "deploy.h"
#include "lib_io.h"
#include "lib_time.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string>

#include "general_head.h"
#include "data.h"
#include "SA.h"
#include "analysis.h"
#include "print_result.h"
#include "utils.h"

/************ 局部变量 *************/
static solutionPack deployPlan;

void deploy_server(char* inLines[MAX_IN_NUM], int inLineNum, const char * const filename)
{
	std::string res;
	
	srand(clock());
	printf("----------------------------------------\n");
	printf("现在是 %d s\n", deployClock);
	if(deployClock==0){
		readFile(inLines);
		initGraph();
		getMaxCostAndMaxServCap();
		getMinCostServerLevel();	
		getAllConsumerMaxFlow();
		deployPlan.solution.resize(vertexNum);
		printf("----------------------------------------\n");
		// myMoney = 1500000;
	}
	roundReadFile(inLines);

	// myMoney+= deployPlan.roundProfit;
	// myMoney-= deployPlan.serverChangeCost;
	solutionPack basicSolution = deployPlan;
	solutionPack lastSolution = deployPlan;
    runAnalysisMap(basicSolution, deployPlan);
	// runAnalysisMap2(basicSolution, deployPlan);

	if(deployClock == 0 /*|| deployClock == 300*/){
		SA_config config;
		config.check_myMoney = false;
		config.change_consumer = false;
		config.sale_server = false;
		int j = 0;
		for(auto i: consumerAnalysisMultimap){
			j++;
			if (j >= (deployPlan.consumerNeedMap.size() + consumerAnalysisMultimap.size()) * 0.5)
				break;
			int need = i.first;
			int id = i.second;
			if(basicSolution.consumerNeedMap.count(id) == 0){
				basicSolution.consumerNeedMap[id] = need;
				// basicSolution.serverLevel[id] = minCostServerLevel;
				basicSolution.serverLevel[id] = serverMaxLevel.at(id);
				basicSolution.solution[id] = true;
			}
		}

		deployPlan = SA(basicSolution, 800, 8.5 * 1000, config, lastSolution);		
		printSAresult(deployPlan, lastSolution);
		printMinFlowResult(res, superS, superT, consumerInfoForPrint, deployPlan.serverLevel, deployPlan.zeroFlowServer);
		write_result(res.c_str(), filename);
	}
	else{
		SA_config config;
		config.sale_server = false;
		deployPlan = SA(basicSolution, 80000, 9 * 1000, config, deployPlan);
		printSAresult(deployPlan, lastSolution);
		printMinFlowResult(res, superS, superT, consumerInfoForPrint, deployPlan.serverLevel, deployPlan.zeroFlowServer);
		write_result(res.c_str(), filename);
	}

	deployClock += 10;
}
