#ifndef _DATA_H
#define _DATA_H

/********************************  readFile *******************************/

/*
函数描述
readFile 在第一次部署服务器的时候调用该函数，读入图的不变数据，
不包括余额和节点被分配的情况，因为这些数据每次部署服务器都需要重新读入
*/

/*全局变量*/
extern vector<vector<int>> edges;
extern int vertexNum, edgeNum, consumerNum, competitorNum;
extern int averageServerCost;
extern vector<vector<int>> consumerInfo;
extern map<int, int> consumerInfoForPrint;
extern vector<vector<int>> hardwareLevel;
extern vector<int> deployCostEachNode;
extern int fee;
extern void readFile(char **topo);


/********************************  roundReadFile  *******************************/

/*
函数描述
每轮读取节点的部署情况和余额情况
*/

/*全局变量*/
extern int myMoney;
extern int myID;
extern int last_myMoney ;

void roundReadFile(char **topo);
/********************************  initGraph  *******************************/

/* 
函数描述
建图，第一次部署服务器时调用 
*/

/*全局变量*/
extern unordered_map<int, vector<int>> edgesMap;
extern int superS;
extern int superT;
void initGraph();

/********************  获取最大服务器流量与费用 **********************/

/********** 全局变量 **********/
extern int maxServerCapacity;
extern int maxServerLevel;

void getMaxCostAndMaxServCap();

/********************************  runMinCostFlow  *******************************/

/*全局变量*/
extern int minCostFlowCount ;
int runMinCostFlow(map<int, int> &capacityMap, map<int, int> &consumerNeedMap);
int superS_arc_flow(int serverNodeId);
pair<int, int> getHardwareLevelInfoByFlow(int flow);



/********* 其他全局变量 **********/
extern int deployClock;


/************** 获取毫秒计时 ****************/
long getMsClock();


/***************  *****************/
/****************** 猜测档位 **********************/

/*全局变量*/
extern int minCostServerLevel;

void getMinCostServerLevel();


/****************** 获取汇点的流量 ******************/

int superT_arc_flow(int consumerNodeId);

/************ 获取节点最大需求 ************/

extern map<int, int> consumerMaxNeed;
extern map<int, int> serverMaxLevel;

void getAllConsumerMaxFlow(); //节点Id：入度

#endif