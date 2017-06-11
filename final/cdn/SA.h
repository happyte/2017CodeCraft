#ifndef _SA_H
#define _SA_H

typedef struct
{
    vector<bool> solution;
    map<int, int> serverLevel; 
    map<int, int> serverFlow;
    map<int, int> consumerNeedMap;
    map<int, int> serverDeployTime;
    set<int> zeroFlowServer;
    bool feasible = true;
    int roundProfit = 0;
    int serverChangeCost = 0;
    int flowCost;
    int totalProfit = 0;
    int fixProfit = 0;
    int fixTotalProfit = 0;
} solutionPack;

typedef struct
{
    bool check_myMoney = true;
    bool change_server = true;
    bool change_consumer = true;
    bool sale_server = true;
} SA_config;

/******************** SA主函数 ***********************/

solutionPack SA(solutionPack &basicSolution, const double T, long timeLimit, SA_config config, const solutionPack lastSolution);

/****************** 获取一个直连解 ********************/
void getDirectSolution(solutionPack &solution);


/******************* 打印每一轮部署情况 *********************/
void printSAresult(const solutionPack &deployPlan, const solutionPack &lastSolution);

#endif