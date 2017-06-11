#include "deploy.h"
#include "data.h"
#include "network_simplex.h"
#include "utils.h"


//不需经常更改的代码


/********************************  全局变量 *******************************/
shared_ptr<NetworkSimplex> _networkSimplex;
char **topo;
int vertexNum, edgeNum, consumerNum, costPerServer;
int totalConsumerNeed = 0;
vector<vector<int>> edges;
vector<vector<int>> consumerInfo;
unordered_map<int, vector<int>> edgesMap;
map<int, int> consumerInfoForPrint;
vector<vector<int>> hardwareLevel;
int maxServerCapacity;
int maxServerLevel = 0;
vector<int> deployCostEachNode;

unordered_map<int, int> capacity;
unordered_map<int, int> cost;
int nodeIdMap[V];
int superT;
int superS;
map<int, map<int, int>> superS_arc;
long beginTime;

int MAX_COST;
int globalCount = 0;
int averageServerCost =0;


solutionPack directConnSolution;
solutionPack optSolution;
string result;
map<int, HardwareUsedInfo> optSoluServerInfo;

struct RunInfo
{
    int addServTotal = 0, validAdd = 0;
    int delServTotal = 0, validDel = 0;
    int chgServToUnSerNeigbTotal = 0, validChgToUnSerNeigb = 0;
    int chgServToUnSerUnNeighbTotal = 0, validChgToUnSerUnNeighb = 0;
    int chgOneServToTwoTotal = 0, validChgOneServToTwo = 0;
    int SA_findOpt = 0;

    int gaMutaTotal = 0, validGaMuta = 0;
    int gaCrOverTotal = 0, validGaCrOver = 0;
    int GA_findOpt = 0;

    int newSoluAreDeadSoluTimes = 0;
    int newSoluAreDeadCauseBy[8] = {0};
    int deadSoluCount = 0;

    LastOprt lastOprt = INIT;

    void addValidOperation();
    void printRunInfo();
};

void RunInfo::printRunInfo()
{
    cout << "【SA】";
    cout << "AddSer: " << validAdd << "/" << addServTotal << "\t";
    cout << "DelSer: " << validDel << "/" << delServTotal << "\t";
    cout << "ChgSer2UnSerNgb: " << validChgToUnSerNeigb << "/" << chgServToUnSerNeigbTotal << "\t";
    cout << "ChgSer2UnSerUnNgb: " << validChgToUnSerUnNeighb << "/" << chgServToUnSerUnNeighbTotal << "\t";
    cout << "ChgOneTo2: " << validChgOneServToTwo << "/" << chgOneServToTwoTotal << "\t";

    //    cout << "【GA】";
    //    cout << "Mutate:" << validGaMuta << "/" << gaMutaTotal << "\t";
    //    cout << "CrsOver:" << validGaCrOver << "/" << gaCrOverTotal << "\t";
    //    cout << "GaFind: " << GA_findOpt << "\t" << "SaFind: " << SA_findOpt << "\t";
    //    cout << " Mutate:" << newSoluAreDeadCauseBy[GaMuta] << " CrsOver:" << newSoluAreDeadCauseBy[GaCrOver] << "\t";

    //    cout << "NewAreDead: " << newSoluAreDeadSoluTimes << "\t" << "DeadSolu: " << deadSoluCount << "\t";
    //    cout << "CausedBy: " << "Add:" << newSoluAreDeadCauseBy[AddSer] << " Del:" << newSoluAreDeadCauseBy[DelSer];
    //    cout << " Chg2UnSerNeigb:" << newSoluAreDeadCauseBy[ChgSerToUnSerNeighbor];
    //    cout << " Chg2UnSerUnNeigb:" << newSoluAreDeadCauseBy[ChgSerToUnSerUnNeighbor];
    //    cout << " ChgOneTo2:" << newSoluAreDeadCauseBy[ChgOneSerToTwo] << endl;

    cout << "【cost】" << MAX_COST - optSolution.fitness << "\tUsed Time:" << (clock() - beginTime) / 1000 << "ms"
         << endl;
}

void RunInfo::addValidOperation()
{
    switch (lastOprt)
    {
    case AddSer:
        validAdd++;
        break;

    case DelSer:
        validDel++;
        break;

    case ChgSerToUnSerNeighbor:
        validChgToUnSerNeigb++;
        break;

    case ChgSerToUnSerUnNeighbor:
        validChgToUnSerUnNeighb++;
        break;

    case ChgOneSerToTwo:
        validChgOneServToTwo++;
        break;

    case GaMuta:
        validGaMuta++;
        break;

    case GaCrOver:
        validGaCrOver++;
        break;

    case INIT:
    default:
        break;
    }
    lastOprt = INIT;
}

RunInfo runInfo;

pair<int, int> getHardwareLevelInfoByFlow(int flow);

int superS_arc_flow(int serverNodeId)
{
    int arc;
    int flow;
    for (uint i = 0; i < hardwareLevel.size(); i++)
    {
        arc = superS_arc[serverNodeId][i];
        flow = _networkSimplex->getFlow(arc);
        if (flow > 0)
        {
            return flow;
            if (flow != _networkSimplex->getNodeFlow(serverNodeId))
                cout << "ERROR" << endl;
        }
    }
    // cout << "该服务器没有流量" << endl;
    return 0;
}

int getHardwareLevelIdByServerNodeId(int serverNodeId)
{
    int flow = superS_arc_flow(serverNodeId);
    return getHardwareLevelInfoByFlow(flow).first;
}

int getArcFlow(int sourceNodeId, int targetNodeId)
{
    int arc = _networkSimplex->findArc(sourceNodeId, targetNodeId);
    return _networkSimplex->getFlow(arc);
}

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

void getMaxCostAndMaxServCap()
{
    int maxDeployCost = *max_element(deployCostEachNode.begin(), deployCostEachNode.end());
    pair<int, int> maxServer = getServerMaxCapacityAndCost(hardwareLevel);
    maxServerCapacity = maxServer.first;
    maxServerLevel = (int)hardwareLevel.size() - 1;
    int maxHardwareLevelCost = maxServer.second;
    MAX_COST = (maxDeployCost + maxHardwareLevelCost) * vertexNum;
}

int minCostServerLevel;
int adapt_mincost_level;
vector<int> minCostServerLevelVector;
map<double, int> minCostServerLevels;

#define GAP 0.8

void getMinCostServerLevel(int &minCostServerLevelsLen)
{
    double mincost = INT_MAX;
    int mincost_id = 0;
    minCostServerLevelsLen = min(minCostServerLevelsLen, (int)hardwareLevel.size());
    printf("[getMinCostServerLevel]\n");
    for(auto info : hardwareLevel){
        double thiscost = (info[2] + averageServerCost) * 1.0 / info[1];
        minCostServerLevels[thiscost] = info[0];
        printf("%d : %.3f \n", info[0], thiscost);
        if(thiscost < mincost - GAP){
            mincost = thiscost;
            mincost_id = info[0];
        }
    }
    minCostServerLevel = mincost_id;
    adapt_mincost_level = minCostServerLevel;
    printf("猜测最小档位 : %d, 单位费用%.3f\n", mincost_id, mincost);
    printf("最小档位排序： \n");
    auto deleteFisrt = minCostServerLevels.end();
    int deleteCount = 0;
    for(auto i = minCostServerLevels.begin(); i!= minCostServerLevels.end();i++){
        if(deleteCount == minCostServerLevelsLen) deleteFisrt = i;
        deleteCount++;
        printf("\t\t %f:%d \n", i->first, i->second);
    }
    printf("保留%d个档位\n", minCostServerLevelsLen);
    printf("删除%f:%d以及之后的档位\n", deleteFisrt->first, deleteFisrt->second );
    minCostServerLevels.erase(deleteFisrt, minCostServerLevels.end());
    printf("删除后的结果： \n");
    if(minCostServerLevels.size() == 0) printf("删除所有最小档位信息，未采用预选档策略，最小档位采用猜测值\n");
    for(auto i = minCostServerLevels.begin(); i!= minCostServerLevels.end();i++){
        printf("\t\t %f:%d \n", i->first, i->second);
    }

    for(int i=0; i<vertexNum; i++){
        double mincost = INT_MAX;
        int mincost_id = 0;
        // printf("\t 节点%d\n", i);
        for(auto info : hardwareLevel){
            double thiscost = (info[2] + deployCostEachNode[i] ) * 1.0 / info[1];
            // printf("%d : %.3f \n", info[0], thiscost);
            if(thiscost < mincost - GAP){
                mincost = thiscost;
                mincost_id = info[0];
            }
        }
        minCostServerLevelVector.push_back(mincost_id);
        // printf("\t 最小档位 : %d, 最小单位费用%.3f\n", mincost_id, mincost);        
    }
}

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
    cout << "error flow bigger than maxCapacity" << endl;
    return make_pair(-1, -1);
}

void readFile()
{
    //1. 读取文件，并初始化变量:节点数、边数、消费节点数、服务器部署成本
    edges.clear();
    vector<int> lineTmp;
    //读取第一行
    split(topo[0], " ", lineTmp);
    vertexNum = lineTmp.at(0);
    edgeNum = lineTmp.at(1);
    consumerNum = lineTmp.at(2);

    //读取第三行:服务器硬件档次
    lineTmp.clear();
    char *line = topo[2];
    string changeLine1 = "\n";
    string changeLine2 = "\r\n";
    while (strcmp(line, changeLine1.c_str()) != 0 && strcmp(line, changeLine2.c_str()) != 0)
    {
        lineTmp.clear();
        split(line, " ", lineTmp);
        hardwareLevel.push_back(lineTmp);
        line = topo[2 + hardwareLevel.size()];
    }

    //每个节点的部署成本
    int startLine = 3 + hardwareLevel.size();
    for (int i = 0; i < vertexNum; ++i)
    {
        vector<int> line;
        split(topo[startLine + i], " ", line);
        int deployCost = line.at(1);
        deployCostEachNode.push_back(deployCost);
        averageServerCost += deployCost;
    }
    averageServerCost /= vertexNum;
    printf("服务器平均花费：%d\n", averageServerCost);

    //读取链路信息
    startLine += vertexNum + 1;
    for (int i = 0; i < edgeNum; i++)
    {
        vector<int> line;
        split(topo[startLine + i], " ", line);
        edges.push_back(line);
    }
    //读取消费节点信息
    startLine += edgeNum + 1;
    for (int i = 0; i < consumerNum; i++)
    {
        vector<int> line;
        split(topo[startLine + i], " ", line);
        consumerInfo.push_back(line);
        consumerInfoForPrint.insert(make_pair(line[1], line[0]));
    }
}

void initGraph(shared_ptr<NetworkSimplex> &networkSimplex)
{
    //1. 建立图结构
    _networkSimplex = make_shared<NetworkSimplex>();
    networkSimplex = _networkSimplex;

    //2. 添加所有节点
    for (int i = 0; i < vertexNum; i++)
    {
        nodeIdMap[i] = _networkSimplex->addNode();
    }
    //加入超级源点: 编号为 vertexNum   超级汇点编号为：vertexNum + 1
    superS = _networkSimplex->addNode();
    superT = _networkSimplex->addNode();

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
        for (uint j = 0; j < hardwareLevel.size(); j++)
        {
            //超级源点的边是单向的
            int arc = _networkSimplex->addArc(superS, i);
            capacity[arc] = hardwareLevel[j].at(1);
            cost[arc] = INT_MAX;
            superS_arc[i][j] = arc;
        }
    }
    //加入超级汇点对应的边
    for (vector<int> consumer : consumerInfo)
    {
        int consumerNodeId = consumer[1];
        int consumerNeed = consumer[2];
        //超级汇点的边是单向的
        int arc = _networkSimplex->addArc(consumerNodeId, superT);
        capacity[arc] = consumerNeed;
        //更新总需求
        totalConsumerNeed += consumerNeed;
        cost[arc] = 0;
    }
    //生成网络单纯形
    _networkSimplex->reset();
    _networkSimplex->upperMap(capacity).costMap(cost).stSupply(superS, superT, totalConsumerNeed);
    cout << "总需求：" << totalConsumerNeed << endl;
}

