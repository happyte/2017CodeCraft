#ifndef _DATA_H
#define _DATA_H

#include "deploy.h"
#include "data.h"

#define V (10000)

inline double rand01()
{
    double x = double(rand()) / double(RAND_MAX);
    return x;
}

extern char **topo;
extern int vertexNum, edgeNum, consumerNum, costPerServer;
extern int totalConsumerNeed;
extern vector<vector<int>> edges;
extern vector<vector<int>> consumerInfo;
extern unordered_map<int, vector<int>> edgesMap;
extern map<int, int> consumerInfoForPrint;
extern vector<vector<int>> hardwareLevel;
extern int maxServerCapacity;
extern int maxServerLevel;
extern vector<int> deployCostEachNode;

extern unordered_map<int, int> capacity;
extern unordered_map<int, int> cost;
extern int nodeIdMap[V];
extern int superT;
extern int superS;
extern map<int, map<int, int>> superS_arc;
extern long beginTime;

extern int MAX_COST;
extern int globalCount;
extern vector<int> minCostServerLevelVector;

typedef struct
{
    vector<bool> solution;
    map<int, int> serverLevel; //<ServerNodeId, level> 保存服务器档次信息
    map<int, int> serverFlow;
    int fitness = 0;
    int age = 0;
} solutionPack;

struct HardwareUsedInfo
{
    int hwId = -1;
    int hwMaxflow = -1;
    int hwActualflow = -1;
    int hwCost = -1;
};

enum ProblemScale
{
    Small,
    Middle,
    Big
};

enum LastOprt
{
    INIT,
    AddSer,
    DelSer,
    ChgSerToUnSerNeighbor,
    ChgSerToUnSerUnNeighbor,
    ChgOneSerToTwo,
    GaMuta,
    GaCrOver,
};

extern solutionPack directConnSolution;
extern solutionPack optSolution;
extern string result;
extern map<int, HardwareUsedInfo> optSoluServerInfo;
extern int minCostServerLevel;
extern map<double, int> minCostServerLevels;
extern int adapt_mincost_level;

int superS_arc_flow(int serverNodeId);
int getHardwareLevelIdByServerNodeId(int serverNodeId);
int getArcFlow(int sourceNodeId, int targetNodeId);
void getOutgoningArcTargetNodeId(int sourceNodeId, std::vector<int> &result);
pair<int, int> getServerMaxCapacityAndCost(const vector<vector<int>> &hardwareLevel);
void getMaxCostAndMaxServCap();
pair<int, int> getHardwareLevelInfoByFlow(int flow);
void readFile();
void initGraph(shared_ptr<NetworkSimplex> &networkSimplex);
void getMinCostServerLevel(int &minCostServerLevelsLen);

#endif