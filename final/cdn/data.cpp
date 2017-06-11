#include "general_head.h"
#include "utils.h"
#include "data.h"
#include <cassert>
#include <sys/time.h>
/********************************  readFile *******************************/

/*
函数描述
readFile 在第一次部署服务器的时候调用该函数，读入图的不变数据，
不包括余额和节点被分配的情况，因为这些数据每次部署服务器都需要重新读入
*/

/*全局变量*/
vector<vector<int>> edges;
int vertexNum, edgeNum, consumerNum, competitorNum;
int averageServerCost = 0;
vector<vector<int>> consumerInfo;
map<int, int> consumerInfoForPrint;
vector<vector<int>> hardwareLevel;
vector<int> deployCostEachNode;
int fee;
set<int> deployCostSet;

void readFile(char **topo)
{
    //1. 读取文件，并初始化变量:节点数、边数、消费节点数、服务器部署成本
    edges.clear();
    vector<int> lineTmp;
    //读取0行
    split(topo[0], " ", lineTmp);
    vertexNum = lineTmp.at(0);
    edgeNum = lineTmp.at(1);
    consumerNum = lineTmp.at(2);
    competitorNum = lineTmp.at(3);

    //读取2行 服务费
    lineTmp.clear();
    split(topo[2], " ", lineTmp);
    fee = lineTmp.at(0);

    printf("节点数：%d，边数：%d，消费者数：%d，竞争者数：%d，服务费：%d\n", vertexNum, edgeNum, consumerNum, competitorNum, fee);

    //读取4行:服务器硬件档次
    lineTmp.clear();
    char *line = topo[4];
    string changeLine1 = "\n";
    string changeLine2 = "\r\n";
    string changeLine3 = "\0";
    while (strcmp(line, changeLine1.c_str()) != 0 && strcmp(line, changeLine2.c_str()) != 0 && strcmp(line, changeLine3.c_str()) != 0)
    {
        lineTmp.clear();
        split(line, " ", lineTmp);
        hardwareLevel.push_back(lineTmp);
        line = topo[4 + hardwareLevel.size()];
    }

    //每个节点的部署成本
    int startLine = 5 + hardwareLevel.size();
    for (int i = 0; i < vertexNum; ++i)
    {
        vector<int> line;
        split(topo[startLine + i], " ", line);
        int deployCost = line.at(1);
        deployCostEachNode.push_back(deployCost);
        deployCostSet.insert(deployCost);
        averageServerCost += deployCost;
    }
    assert((int)deployCostEachNode.size() == vertexNum);
    averageServerCost /= vertexNum;
    printf("服务器平均花费：%d 档位分别为", averageServerCost);
    for(auto i:deployCostSet){
        printf(" %d ", i);
    }
    printf("\n");

    //读取链路信息
    startLine = 6 + hardwareLevel.size() + vertexNum;
    for (int i = 0; i < edgeNum; i++)
    {
        vector<int> line;
        split(topo[startLine + i], " ", line);
        edges.push_back(line);
    }
    assert((int)edges.size() == edgeNum);
    //读取消费节点信息
    startLine = 7 + hardwareLevel.size() + vertexNum + edgeNum;
    for (int i = 0; i < consumerNum; i++)
    {
        vector<int> line;
        split(topo[startLine + i], " ", line);
        consumerInfo.push_back(line);
        consumerInfoForPrint.insert(make_pair(line[1], line[0]));
    } 
    assert((int)consumerInfo.size() == consumerNum);
}


/********************************  roundReadFile  *******************************/

/*
函数描述
每轮读取节点的部署情况和余额情况
*/

/*全局变量*/
int myMoney = 0;
int myID = 0;
int last_myMoney = 0;

void roundReadFile(char **topo){
    int startLine = 7 + hardwareLevel.size() + vertexNum + edgeNum;
    consumerInfo.clear();
    for (int i = 0; i < consumerNum; i++)
    {
        vector<int> line;
        split(topo[startLine + i], " ", line);
        consumerInfo.push_back(line);
        // printf("%d %d %d %d \n", line.at(0), line.at(1), line.at(2), line.at(3));
        // consumerInfoForPrint.insert(make_pair(line[1], line[0]));
    }

    startLine = 8 + hardwareLevel.size() + vertexNum + edgeNum + consumerNum;
    vector<int> line;
    split(topo[startLine], " ", line);
    myID = line.at(0);
    last_myMoney = myMoney;
    myMoney = line.at(1);

    printf("我的ID：%d，余额：%d\n", myID, myMoney);
}


/********************************  initGraph  *******************************/

/* 
函数描述
建图，第一次部署服务器时调用 
*/

/*全局变量*/
unordered_map<int, vector<int>> edgesMap;
int superS;
int superT;

/*局部变量*/
static shared_ptr<NetworkSimplex> _networkSimplex;
static unordered_map<int, int> capacity;
static unordered_map<int, int> cost;
static map<int, int> superS_arc;
static map<int, int> superT_arc;
static int addFlowArc;
static int delFlowArc;
static int bak_addFlowArc;
static int bak_delFlowArc;


void initGraph()
{
    //1. 建立图结构
    _networkSimplex = make_shared<NetworkSimplex>();

    //2. 添加所有节点
    for (int i = 0; i < vertexNum; i++)
    {
        int nodeid = _networkSimplex->addNode();
        assert(nodeid == i);
    }
    //加入超级源点: 编号为 vertexNum   超级汇点编号为：vertexNum + 1
    superS = _networkSimplex->addNode();
    superT = _networkSimplex->addNode();

    //加入超级源点和超级汇点之间的弧, 用于加速费用流
    addFlowArc = _networkSimplex->addArc(superS, superT);
    delFlowArc = _networkSimplex->addArc(superT, superS);
    capacity[addFlowArc] = _networkSimplex->INF;
    capacity[delFlowArc] = _networkSimplex->INF;
    cost[addFlowArc] = INT_MAX;
    cost[delFlowArc] = INT_MAX;
    //溢流弧
    bak_addFlowArc = _networkSimplex->addArc(superS, superT);
    bak_delFlowArc = _networkSimplex->addArc(superT, superS);
    capacity[bak_addFlowArc] = _networkSimplex->INF;
    capacity[bak_delFlowArc] = _networkSimplex->INF;
    cost[bak_addFlowArc] = INT_MAX/2+1;
    cost[bak_delFlowArc] = INT_MAX/2+1;    


    //加入所有的边：不包括超级源点 和 超级汇点 的边
    for (vector<int> edge : edges)
    {
        int sourceNodeId = edge[0];
        int destNodeId = edge[1];
        int u = edge[2];
        int c = edge[3];
        //正向边
        int arc = _networkSimplex->addArc(sourceNodeId, destNodeId);
        capacity[arc] = u;
        cost[arc] = c;
        //反向边
        int arcReverse = _networkSimplex->addArc(destNodeId, sourceNodeId);
        capacity[arcReverse] = u;
        cost[arcReverse] = c;
        //加入到edgesMap中
        edgesMap[sourceNodeId].push_back(destNodeId);
        edgesMap[destNodeId].push_back(sourceNodeId);
    }
    //加入超级源点
    for (int i = 0; i < superS; i++)
    {
        int arc = _networkSimplex->addArc(superS, i);
        capacity[arc] = _networkSimplex->INF;
        cost[arc] = INT_MAX;
        superS_arc[i] = arc;
        // arc = _networkSimplex->addArc(superS, i);
        // capacity[arc] = _networkSimplex->INF;
        // cost[arc] = INT_MAX/4;
    }
    //加入超级汇点对应的边
    for (vector<int> consumer : consumerInfo)
    {
        int consumerNodeId = consumer[1];
        //超级汇点的边是单向的
        int arc = _networkSimplex->addArc(consumerNodeId, superT);
        capacity[arc] = _networkSimplex->INF;
        cost[arc] = INT_MAX;
        superT_arc[consumerNodeId] = arc;
        // arc = _networkSimplex->addArc(consumerNodeId, superT);
        // capacity[arc] = _networkSimplex->INF;
        // cost[arc] = INT_MAX/4;
    }
    //生成网络单纯形
    _networkSimplex->reset();
    _networkSimplex->upperMap(capacity).costMap(cost);
}


/********************************  runMinCostFlow  *******************************/

/*全局变量*/
int minCostFlowCount = 0;
/* 函数描述 */
/*
给定源点的档位和目标点的需求，求出费用流
*/

static map<int, int> local_cap;
static map<int, int> local_need;
static int last_totalConsumerNeed = 0;


int runMinCostFlow(map<int, int> &capacityMap, map<int, int> &consumerNeedMap)
{
    int flowCost;
    static bool init = false;
    bool modCapNeed = false;
    NetworkSimplex::ProblemType ret;
    
    if(init == false){
        _networkSimplex->init();
        init = true;
    }
    
    minCostFlowCount++;

    // cout << "run min cost flow" << endl;

    /* 计算消费者总需求 */
    int totalConsumerNeed = 0;
    for(auto i:consumerNeedMap){
        totalConsumerNeed += i.second;
    }

    int chgNeed = totalConsumerNeed - last_totalConsumerNeed ;
    int nodeid;
    int arc;
    for (auto i : local_cap)
    {
        if (capacityMap.count(i.first) == 0 || capacityMap.at(i.first) != local_cap.at(i.first))
        {
            if (capacityMap.count(i.first) == 1 && local_cap.count(i.first) == 1 && capacityMap.at(i.first) != local_cap.at(i.first))
            {
                modCapNeed = true;
            }
            nodeid = i.first;
            arc = superS_arc[nodeid];
            _networkSimplex->stCost(INT_MAX, arc);
        }
    }

    for (auto i : local_need)
    {
        if (consumerNeedMap.count(i.first) == 0 || consumerNeedMap.at(i.first) != local_need.at(i.first))
        {
            if (consumerNeedMap.count(i.first) == 1 && local_need.count(i.first) == 1 && consumerNeedMap.at(i.first) != local_need.at(i.first))
            {
                modCapNeed = true;
            }
            nodeid = i.first;
            arc = superT_arc.at(nodeid);
            _networkSimplex->stCost(INT_MAX, arc);
        }
    }

    if(modCapNeed == true){
        _networkSimplex->fixPotential();
        ret = _networkSimplex->directRun();
    }


    if(chgNeed > 0){
        int flow = _networkSimplex->getFlow(addFlowArc);
        assert(flow == 0);
        _networkSimplex->stFlow(chgNeed, addFlowArc);
        _networkSimplex->stUpper(chgNeed, addFlowArc);
        if(_networkSimplex->getArcState(addFlowArc) == NetworkSimplex::STATE_LOWER){
            _networkSimplex->stArcState(NetworkSimplex::STATE_UPPER, addFlowArc);
        }
    }
    else if(chgNeed < 0){
        int flow = _networkSimplex->getFlow(delFlowArc);
        assert(flow == 0);
        _networkSimplex->stFlow(-chgNeed, delFlowArc);
        _networkSimplex->stUpper(-chgNeed, delFlowArc);
        if (_networkSimplex->getArcState(delFlowArc) == NetworkSimplex::STATE_LOWER)
        {
            _networkSimplex->stArcState(NetworkSimplex::STATE_UPPER, delFlowArc);
        }
    }

    for (auto i : capacityMap)
    {
        int nodeid = i.first;
        int arc = superS_arc[nodeid];
        int level = i.second;
        int cap = hardwareLevel.at(level).at(1);
        if(local_cap.count(nodeid) == 0 || capacityMap.at(i.first) != local_cap.at(i.first)){
            _networkSimplex->stCost(0, arc);
            _networkSimplex->stUpper(cap, arc);
        }
    }    

    for (auto i : consumerNeedMap)
    {
        nodeid = i.first;
        arc = superT_arc.at(nodeid);
        int need = i.second;
        if (local_need.count(nodeid) == 0 || consumerNeedMap.at(i.first) != local_need.at(i.first))
        {
            _networkSimplex->stCost(0, arc);
            _networkSimplex->stUpper(need, arc);
        }
    }

    /* 计算费用流 */
    _networkSimplex->fixPotential();
    ret = _networkSimplex->directRun();

    /* 计算费用 */
    if(ret == NetworkSimplex::OPTIMAL) 
        flowCost = _networkSimplex->totalCost();
    else flowCost = -1;

    assert(flowCost < INT_MAX/10);
    


    last_totalConsumerNeed = totalConsumerNeed;
    local_cap = capacityMap;
    local_need = consumerNeedMap;

    return flowCost;
}

/****************** 获取源点的流量 ******************/

int superS_arc_flow(int serverNodeId)
{
    int arc;
    int flow = 0;

    arc = superS_arc[serverNodeId];
    flow = _networkSimplex->getFlow(arc);

    // cout << "该服务器没有流量" << endl;
    return flow;
}

/****************** 获取汇点的流量 ******************/

int superT_arc_flow(int consumerNodeId)
{
    int arc;
    int flow = 0;

    arc = superT_arc[consumerNodeId];
    flow = _networkSimplex->getFlow(arc);

    // cout << "该服务器没有流量" << endl;
    return flow;
}


/****************** 获取vector中最大服务器流量与费用 **********************/
pair<int, int> getServerMaxCapacityAndCost(const vector<vector<int>> &hardwareLevel)
{
    int maxCost = 0, maxCapacity = 0;
    for (uint i = 0; i < hardwareLevel.size(); i++)
    {
        vector<int> tmp = hardwareLevel[i];
        if (maxCost < tmp[2])
        {
            maxCost = tmp[2];
        }
        if (maxCapacity < tmp[1])
        {
            maxCapacity = tmp[1];
        }
    }
    return make_pair(maxCapacity, maxCost);
}


/********************  获取最大服务器流量与费用 **********************/

/********** 全局变量 **********/
int maxServerCapacity;
int maxServerLevel;

void getMaxCostAndMaxServCap()
{
    pair<int, int> maxServer = getServerMaxCapacityAndCost(hardwareLevel);
    maxServerCapacity = maxServer.first;
    maxServerLevel = (int)hardwareLevel.size() - 1;
}


/***************** 根据流量获取服务器档位信息 *****************************/

pair<int, int> getHardwareLevelInfoByFlow(int flow)
{
    for (uint i = 0; i < hardwareLevel.size(); ++i)
    {
        vector<int> hardwareInfo = hardwareLevel[i];
        if (flow <= hardwareInfo[1])
        {
            return make_pair(hardwareInfo[0], hardwareInfo[2]);
        }
    }
    // cout << "error flow bigger than maxCapacity " << flow << endl;
    // return make_pair(-1, -1);
    return make_pair(maxServerLevel, maxServerCapacity);
}


/********************** 获取两个节点间的流量 ***************************/
int getArcFlow(int sourceNodeId, int targetNodeId)
{
    int arc = _networkSimplex->findArc(sourceNodeId, targetNodeId);
    return _networkSimplex->getFlow(arc);
}

/**************** 获取有流量的出边 *********************/
void getOutgoningArcTargetNodeId(int sourceNodeId, std::vector<int> &result)
{
    result.clear();
    for (auto targetNodeId : _networkSimplex->out(sourceNodeId))
    {
        int arc = _networkSimplex->findArc(sourceNodeId, targetNodeId);
        if (_networkSimplex->getFlow(arc) > 0)
        {
            result.push_back(targetNodeId);
        }
    }
}


/********* 其他全局变量 **********/
int deployClock;



/************** 获取毫秒计时 ****************/
long getMsClock(){
    struct timeval tv;
	gettimeofday(&tv,NULL);
	//printf("millisecond:%ld\n",tv.tv_sec*1000 + tv.tv_usec/1000);  //毫秒
    return tv.tv_sec*1000 + tv.tv_usec/1000;
    // return clock() / 1000;
}


/****************** 猜测档位 **********************/

/*全局变量*/
int minCostServerLevel;

/*局部变量*/
static map<double, int> minCostServerLevels;
#define GAP 3

void getMinCostServerLevel()
{
    double mincost = INT_MAX;
    int mincost_id = 0;
    printf("[getMinCostServerLevel]\n");
    for(auto info : hardwareLevel){
        double thiscost = (info[2] + averageServerCost) * 1.0 / info[1];
        minCostServerLevels[thiscost] = info[0];
        printf("档位 %d  流量单价 %.3f 流量:%d 总费用 %d 档位费用 %d\n", info[0], thiscost, info[1], info[2] + averageServerCost, info[2]);
        if(thiscost < mincost - GAP){
            mincost = thiscost;
            mincost_id = info[0];
        }
    }
    minCostServerLevel = mincost_id;
    printf("猜测最小档位 : %d, 单位费用%.3f\n", mincost_id, mincost);
}



static bool nodeIsConsumer(int nodeId)
{
    for (auto ite : consumerInfo){
        int id = ite.at(1);
        if(id == nodeId)
            return true;
    }
    return false;
}

/************ 获取节点最大需求等信息 ************/

map<int, int> consumerMaxNeed;
map<int, int> consumerMaxNeed2;
map<int, int> serverMaxCap;
map<int, int> serverMaxLevel;
map<int, int> averageEachConsumerCost;

void getAllConsumerMaxFlow() //节点Id：入度
{

    // for (auto ite : edges){
    //     // int source = ite.at(0);
    //     int dest = ite.at(1);
    //     int capicity = ite.at(2);
    //     // int cost = ite.at(3);
    //     if(nodeIsConsumer(dest)){
    //         consumerMaxNeed[dest] += capicity;
    //     }
    // }

    for (auto i:consumerInfo)
    {
        int id = i.at(1);
        vector<int> out = _networkSimplex->out(id);
        int cap = 0;
        int lineNum = 0;
        int cost = 0;
        for(auto target:out)
        {
            if(target != superS && target != superT){
                cap += _networkSimplex->getUpper( _networkSimplex->findArc(id, target) );
                cost += _networkSimplex->getCost( _networkSimplex->findArc(id, target) );
                lineNum ++;
            }
        }
        consumerMaxNeed[id] = cap ;
        averageEachConsumerCost[id] = cost / lineNum;
    }

    for( int i=0;i<vertexNum;i++){
        int id = i;
        vector<int> out = _networkSimplex->out(id);
        int cap = 0;
        for(auto target:out)
        {
            if(target != superS && target != superT){
                cap += _networkSimplex->getUpper( _networkSimplex->findArc(id, target) );
            }
        }
        serverMaxCap[id] = cap;
        auto info = getHardwareLevelInfoByFlow(cap);
        serverMaxLevel[id] = min(info.first, minCostServerLevel);
        // printf("id %d 容量 %d 最高等级 %d \n", id , cap, info.first);
    }


    // for (auto ite : averageEachConsumerCost){
    //     int id = ite.first;
    //     int maxNeed = ite.second;
    //     int maxNeed2 = serverMaxCap.at(id);
    //     printf("id:%d \t cost :%d, maxNeed %d  maxNeedSet %d", id, maxNeed, maxNeed2, fee / maxNeed);
    //     if(maxNeed > fee / maxNeed){
    //         printf("超出 !!!");
    //     }
    //     printf("\n");
    // }


}