#include "deploy.h"
#include "general_head.h"
#include "data.h"

using namespace std;

#define maxint 0x3f3f3f3f
#define V 5000

/* 以下数据用于保存输出数据 */
static int recordPrintfFlowCache[V][V];
static int Print_Start_S;
static int Print_End_T;
static unordered_map<int, unordered_map<int, int>> arcFlowCache;
static map<int, string> zeroFlowLine;


/* 引用函数 */
extern int getArcFlow(int sourceNodeId, int targetNodeId);
extern void getOutgoningArcTargetNodeId(int sourceNodeId, std::vector<int> &result);

/* 返回这个节点有最大流量的一条边 */
static int getAFlowEdge(int node, int &flow)
{
    vector<int> nextNodes;
    getOutgoningArcTargetNodeId(node, nextNodes);
    int returnFlowEdgeNode = -1;
    int fullFlow = 0;
    for (auto nextNode : nextNodes)
    {
        fullFlow = arcFlowCache[node][nextNode];
        if (fullFlow == 0)
            fullFlow = arcFlowCache[node][nextNode] = getArcFlow(node, nextNode);
        if (recordPrintfFlowCache[node][nextNode] < fullFlow)
        {
            returnFlowEdgeNode = nextNode;
            flow = fullFlow - recordPrintfFlowCache[node][nextNode];
            break;
        }
    }
    return returnFlowEdgeNode;
}

/* 获取从超级原点到超级汇点的一条流量链路，将该流量链路上的每一个节点存入数组flowLine，并返回链路的最小流量值 */
/* 当所有流量都被扣完，无法获取流量链路，返回maxint */
static int getAFlowLine(vector<int> &flowLine)
{
    int minFlow = maxint;
    int flow = 0;
    int nextNode;
    int node = Print_Start_S;
    flowLine.push_back(Print_Start_S);
    while (1)
    {
        nextNode = getAFlowEdge(node, flow);
        if (nextNode == -1)
            break;
        if (flow < minFlow)
            minFlow = flow;
        flowLine.push_back(nextNode);
        node = nextNode;
    }
    return minFlow;
}

/* 将本地图加入该链路的流量 */
static void addThisFlowLine(vector<int> flowLine, int minFlow)
{
    for (unsigned int i = 0; i < flowLine.size() - 1; i++)
    {
        recordPrintfFlowCache[flowLine[i]][flowLine[i + 1]] += minFlow;
    }
}

/* 打印当前增加过最窄流量链路到缓存中 */
static void printThisFlowLine(vector<int> flowLine, int minFlow, int &resultInOneLineNum,
                              string &resultInOneLine, const map<int, int> &consumerID,const map<int, int> &serverLevel)
{
    string stringBuff;
    string stringBuff2;
    char str[100];
    char str2[100];
    resultInOneLineNum++;
    for (unsigned int i = 1; i < flowLine.size() - 1; i++)
    {
        sprintf(str, "%d ", flowLine[i]);
        stringBuff += str;
    }
    stringBuff2 = stringBuff;
    int hardwareLevelId = serverLevel.at(flowLine[1]);
    sprintf(str, "%d %d %d\n", consumerID.at(flowLine[flowLine.size() - 2]), hardwareLevelId, minFlow);
    stringBuff += str;
    sprintf(str2, "%d %d 0\n", consumerID.at(flowLine[flowLine.size() - 2]), hardwareLevelId);
    stringBuff2 += str2;
    zeroFlowLine[flowLine[1]] = stringBuff2;
    resultInOneLine += stringBuff;
}

/* 打印所有最小流量到缓存中 */
void printMinFlowResult(std::string &resultInOneLine, int S_ID, int T_ID, const map<int, int> &consumerID,
                        const map<int, int> &serverLevel, const set<int> &zeroServer)
{
    int minFlow;
    char str[100];
    int resultInOneLineNum = 0;
    resultInOneLine.clear();
    Print_Start_S = S_ID;
    Print_End_T = T_ID;
    memset(recordPrintfFlowCache, 0, sizeof(recordPrintfFlowCache));
    arcFlowCache.clear();
    while (1)
    {
        vector<int> flowLine;
        minFlow = getAFlowLine(flowLine);
        if (minFlow == maxint)
            break;
        printThisFlowLine(flowLine, minFlow, resultInOneLineNum, resultInOneLine, consumerID, serverLevel);
        addThisFlowLine(flowLine, minFlow);
    };

    for(auto i:zeroServer){
        resultInOneLine += zeroFlowLine.at(i);
    }

    sprintf(str, "%d\n\n", resultInOneLineNum + (int)zeroServer.size());
    resultInOneLine = str + resultInOneLine;

    // for(auto i: zeroFlowLine){
    //     printf("%s", i.second.c_str());
    // }
}
