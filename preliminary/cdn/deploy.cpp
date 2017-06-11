#include "deploy.h"
#include "network_simplex.h"
#include "general_head.h"
#include "utils.h"
#include "print_result.h"
#include <unordered_map>
#include <stdio.h>
#include <time.h>
#include <random>
#include <algorithm>
#include <climits>
#include <queue>
#include <bitset>
#include <unordered_set>


using namespace std;
using namespace xbl_graph;

#define MAX_RUN_TIME (88 * 1000 * 1000)                 //最大运行时间
#define MAX_RETRY_TIME (7000)                         //最优解未变化最大允许次数
#define MY_MAX_INT (0x3f3f3f3f)
#define V 1200

#define CROSS_RATE_PLUE             (0.005)             //交叉增加系数
#define CROSS_RATE_DIVISION         (2)                 //交叉惩罚系数
#define MAX_AGE                     (1000)              //脱离局部最优系数
#define PRINT_CLOCK_MS              (1000)              //打印频率
#define POP_SIZE                    (30)                //解队列长度，种群大小

char **topo;

/********************************  全局变量 *******************************/

shared_ptr<NetworkSimplex> networkSimplex;

int vertexNum, edgeNum, consumerNum, costPerServer;
int totalConsumerNeed = 0;
vector<vector<int>> edges;
vector<vector<int>> consumerInfo;
unordered_map<int, vector<int> > edgesMap;
map<int, int> consumerInfoForPrint;
set<int> consumerSet;

unordered_map<int, int> capacity;
unordered_map<int, int> cost;
int nodeIdMap[V];
int superT;
int superS;
int superS_arc[V];

typedef struct solutionPack
{
    int cost = INT_MAX;
    vector<int> solution;
    vector<int> solutionAlt;
    bitset<V> bits;
    int age = 0;
} solutionPack;

struct infosa
{
    int cross = 0;
    int mutate = 0;
    int cross_good = 0;
    int mutate_good = 0;
    int cross_better = 0;
    int mutate_better = 0;
    int last_operation = 0;
    int operation[10] = {0};
};

struct infosa infoSA;

struct infomcf
{
    int cache = 0;
    int runtimes = 0;
};

struct infomcf infoMinCostFlow;


/**
* *************************************  Public Methods ***************************************
*/

int getArcFlow(int sourceNodeId, int targetNodeId)
{
    int arc = networkSimplex->findArc(sourceNodeId, targetNodeId);
    return networkSimplex->flow(arc);
}

void getOutgoningArcTargetNodeId(int sourceNodeId, std::vector<int> &result)
{
    result.clear();
    for (auto targetNodeId: networkSimplex->out(sourceNodeId))
    {
		int arc = networkSimplex->findArc(sourceNodeId, targetNodeId);
        if (networkSimplex->flow(arc) > 0)
        {
            result.push_back(targetNodeId);
        }
    }
}

/**
* *************************************  Private Methods ***************************************
*/

static void readFile()
{
    //1. 读取文件，并初始化变量:节点数、边数、消费节点数、服务器部署成本
    edges.clear();
    vector<int> lineTmp;
    //读取第一行
    split(topo[0], " ", lineTmp);
    vertexNum = lineTmp.at(0);
    edgeNum = lineTmp.at(1);
    consumerNum = lineTmp.at(2);
    //读取第三行
    lineTmp.clear();
    split(topo[2], " ", lineTmp);
    costPerServer = lineTmp.at(0);

    //读取链路信息
    for (int i = 0; i < edgeNum; i++)
    {
        vector<int> line;
        split(topo[4 + i], " ", line);
        edges.push_back(line);
    }
    //读取消费节点信息
    for (int i = 0; i < consumerNum; i++)
    {
        vector<int> line;
        split(topo[5 + edgeNum + i], " ", line);
        consumerInfo.push_back(line);
        consumerInfoForPrint.insert(make_pair(line[1], line[0]));
        consumerSet.insert(line[1]);
    }
}

static void initGraph()
{
	//1. 建立图结构
    networkSimplex = make_shared<NetworkSimplex>();
    //2. 根据变量，建立ListDigraph, 超级源点, 超级汇点
    for (int i = 0; i < vertexNum; i++)
    {
        nodeIdMap[i] = networkSimplex->addNode();
    }
    //加入超级源点: 编号为 vertexNum   超级汇点编号为：vertexNum + 1
    superS = networkSimplex->addNode();
    superT = networkSimplex->addNode();
    cout << superS << " " << superT << endl;
    //加入所有的边：不包括超级源点 和 超级汇点 的边
    for (vector<int> edge : edges)
    {
        int sourceNodeId = edge[0];
        int destNodeId = edge[1];
        int u = edge[2];
        int c = edge[3];
        //正向边
        int arc = networkSimplex->addArc(sourceNodeId, destNodeId);
        capacity[arc] = u;
        cost[arc] = c;
        //反向边
        int arcReverse = networkSimplex->addArc(destNodeId, sourceNodeId);
        capacity[arcReverse] = u;
        cost[arcReverse] = c;
        //加入到edgesMap中
        edgesMap[sourceNodeId].push_back(destNodeId);
        edgesMap[destNodeId].push_back(sourceNodeId);
    }
    //加入超级源点
    for (int i = 0; i < superS; i++)
    {
        //超级源点的边是单向的
        int arc = networkSimplex->addArc(superS, i);
        capacity[arc] = 0;
        cost[arc] = 0;
        superS_arc[i] = arc;
    }
    //加入超级汇点对应的边
    for (vector<int> consumer : consumerInfo)
    {
        //int consumerId = consumer[0];//暂时没用
        int consumerNodeId = consumer[1];
        int consumerNeed = consumer[2];
        //超级汇点的边是单向的
        int arc = networkSimplex->addArc(consumerNodeId, superT);
        capacity[arc] = consumerNeed;
        //更新总需求
        totalConsumerNeed += consumerNeed;
        cost[arc] = 0;
    }
    //生成网络单纯形
    networkSimplex->reset();
    networkSimplex->costMap(cost).stSupply(superS, superT, totalConsumerNeed);
}

/*********************** 计算最小费用 ***************************/
int runMinCostFlow(solutionPack &solution, bool cache = true)
{
    static unordered_map<bitset<V>, int> solutionCache;
    int cost;
    if (solutionCache[solution.bits] != 0 && cache)
    {
        infoMinCostFlow.cache ++;
        return solutionCache[solution.bits];
    }
    //超级源点的边设置为有容量
    for (auto i : solution.solution)
    {
        int arc = superS_arc[i];//findArc(graph, superS, graph.nodeFromId(i));
        capacity[arc] = MY_MAX_INT;
    }
    //5. 针对解： 调用方法求出ListDigraph的最小费用
    infoMinCostFlow.runtimes++;
    NetworkSimplex::ProblemType ret = networkSimplex->upperMap(capacity).run();
    if (ret == NetworkSimplex::ProblemType::OPTIMAL)
        cost = networkSimplex->totalCost() + costPerServer * solution.solution.size();
    else
        cost = INT_MAX;
    //计算完毕，恢复超级源点的边为默认状态
    for (auto i : solution.solution)
    {
        int arc = superS_arc[i];//findArc(graph, superS, graph.nodeFromId(i));
        capacity[arc] = 0;
    }
    //记录到缓存中
    solutionCache[solution.bits] = cost;
    solution.cost = cost;
    return cost;
}

static solutionPack _basicSolution;

void SA_SetBasicSolution(const solutionPack &basicSolution)
{
    _basicSolution = basicSolution;
}
std::random_device rand_dev;
std::mt19937_64 rng(rand_dev());
std::uniform_real_distribution<> proba(0, 1);

template <typename T>
inline T uniform(T min, T max)
{
    return min + proba(rng) * (max - min);
}

inline static void forwordChg(vector<int> &vec, vector<int> &vecAlt)
{
    int vecSize = vec.size();
    int pos;
    if (vecSize == 0)
        return;
    pos = uniform(0, vecSize);
    vecAlt.push_back(vec[pos]);
    vec[pos] = vec[vecSize - 1];
    vec.pop_back();
}

inline static void backChg(vector<int> &vec, vector<int> &vecAlt)
{
    int vecAltSize = vecAlt.size();
    int pos;
    if (vecAltSize == 0)
        return;
    pos = uniform(0, vecAltSize);
    vec.push_back(vecAlt[pos]);
    vecAlt[pos] = vecAlt[vecAltSize - 1];
    vecAlt.pop_back();
}

inline static void swapChg(vector<int> &vec, vector<int> &vecAlt)
{
    int vecSize = vec.size();
    int vecAltSize = vecAlt.size();
    int pos1,pos2,temp;
    // printf("chg!\n");
    if (vecAltSize == 0)
        return;
    if (vecSize == 0)
        return;
    pos1 = uniform(0, vecSize);
    pos2 = uniform(0, vecAltSize);
    temp = vecAlt[pos2];
    vecAlt[pos2] = vec[pos1];
    vec[pos1] = temp; 
}

int getRandomNeighbor(int i)
{
    int pos = uniform(0, (int)edgesMap[i].size());
    return edgesMap[i][pos];
}

void chgVector(vector<int> &vec, vector<int> &vecAlt, bitset<V> &bits)
{
    float r = uniform(0.0, 1.0);
    /* 正向一交换 */
    /* 即删除 */
    if (r > 2.0 / 3.0)
    {
        forwordChg(vec, vecAlt);
        bits.reset();
        for(auto i:vec) bits[i] = 1;
        infoSA.last_operation = 1;
    }
    /* 反向一交换 */
    /* 即增加 */
    else if (r > 1.0 / 3.0)
    {
        backChg(vec, vecAlt);
        bits.reset();
        for(auto i:vec) bits[i] = 1;
        infoSA.last_operation = 2;
    }
    /* 正向一交换反向一交换 */
    /* 即改变 */
    else if(r > 0.0 / 3.0)
    {
        int start_pos = uniform(0, vertexNum);
        int chg_pos = 0;
        for(int i=0;i<vertexNum;i++)
        {
            chg_pos = (start_pos+i)%vertexNum;
            if(bits[chg_pos] == 1){
                break;
            }
        }
        int near = getRandomNeighbor(chg_pos);
        bits[chg_pos] = 0;
        bits[near] = 1;
        vec.clear();
        vecAlt.clear();
        for(int i=0; i<vertexNum; i++)
        {
            if(bits[i] == 1) vec.push_back(i);
            else vecAlt.push_back(i);
        }
        infoSA.last_operation = 3;
    }
    // else{
    //     // swapChg(vec, vecAlt);
    //     forwordChg(vec, vecAlt);
    //     backChg(vec, vecAlt);
    //     bits.reset();
    //     for(auto i:vec) bits[i] = 1;
    //     infoSA.last_operation = 4;
    // }
}

void crossOver(const bitset<V> &bitsetA, const bitset<V> &bitsetB, bitset<V> &crossBits)
{
    float r;
    for(int i=0; i<V; i++)
    {
        r = uniform(0.0, 1.0);
        if(r>0.5)   crossBits[i] = bitsetA[i];
        else        crossBits[i] = bitsetB[i];
    }
    infoSA.last_operation=5;
}

static unordered_set<bitset<V> > searchedSolution;

/* 基于已有的一个解，获取一个新解 */
/* 如果这个解已经产生过，那么返回初始解（通常非常差） */
solutionPack getNewSolution(const solutionPack &originSolution)
{
    infoSA.mutate++;
    solutionPack newSolution = originSolution;
    chgVector(newSolution.solution, newSolution.solutionAlt, newSolution.bits);
    if(searchedSolution.count(newSolution.bits)) return _basicSolution;
    newSolution.cost = runMinCostFlow(newSolution);
    newSolution.age = 0;
    // searchedSolution.insert(newSolution.bits);
    return newSolution;
}

/* 基于已有的两个解，获取一个新解 */
solutionPack getNewSolution(const solutionPack &originSolutionA, const solutionPack &originSolutionB)
{
    infoSA.cross++;
    solutionPack newSolution;
    int solutionSize = originSolutionA.solution.size() + originSolutionA.solutionAlt.size();
    crossOver(originSolutionA.bits, originSolutionB.bits, newSolution.bits);
    if(searchedSolution.count(newSolution.bits)) return _basicSolution;
    for(int i=0;i<solutionSize;i++)
    {
        if(newSolution.bits[i] == 1) newSolution.solution.push_back(i);
        else newSolution.solutionAlt.push_back(i);
    }
    newSolution.cost = runMinCostFlow(newSolution);
    newSolution.age = 0;
    // searchedSolution.insert(newSolution.bits);
    return newSolution;
}

bool LessSort(solutionPack a, solutionPack b)
{
    if(a.cost < b.cost) return true;
    if(a.cost == b.cost) {
        if(a.bits == b.bits) return a.age > b.age;
        else return a.bits.to_string() < b.bits.to_string();
    }
    return false;
}
bool uniqueSort(solutionPack a, solutionPack b) { return (a.bits == b.bits); }

unsigned int popLimit = POP_SIZE;
float crossRate = 0.000001;

void SA(solutionPack &optSolution, long timeLimit_ns = MAX_RUN_TIME, int max_try = MAX_RETRY_TIME)
{
    long startTime_ns = clock();
    long printClock = 0;
    vector<solutionPack> solutions;
    if (_basicSolution.solution.size() == 0)
        return;
    optSolution = _basicSolution;
    optSolution.cost = runMinCostFlow(optSolution);
    solutions.push_back(optSolution);
    for (int retry = 0; retry <  max_try * (int)popLimit && clock() - startTime_ns < timeLimit_ns; )
    {
        for (auto eachSolution = solutions.begin(); eachSolution != solutions.end(); eachSolution++)
        {
            eachSolution->age++;
            solutionPack tempSolution = getNewSolution(*eachSolution);
            if (tempSolution.cost < solutions[solutions.size() - 1].cost || (tempSolution.cost < INT_MAX && solutions.size() < popLimit))
            {
                if (optSolution.cost > tempSolution.cost)
                {
                    retry = 0;
                    infoSA.mutate_better++;
                    infoSA.operation[infoSA.last_operation] ++;
                    optSolution = tempSolution;
                    printf("[SA] 通过变异获得最优解: %d， 当前时间 %ld ms\n", optSolution.cost,  (clock() - startTime_ns) / 1000);
                }
                solutions.push_back(tempSolution);
                sort(solutions.begin(), solutions.end(), LessSort);
                // for(auto i:solutions) printf("%d(%d)--", i.cost, i.age);
                // printf("\n");
                infoSA.mutate_good++;
                
                break;
            }
            else
                if(tempSolution.cost != _basicSolution.cost) retry++;
        }
        float r = uniform(0.0, 1.0);
        if (r < crossRate)
        {
            int S1 = uniform(0, (int)solutions.size());
            int S2 = uniform(0, (int)solutions.size());
            solutionPack tempSolution = getNewSolution(solutions[S1], solutions[S2]);
            if (tempSolution.cost < solutions[solutions.size() - 1].cost)
            {
                if (optSolution.cost > tempSolution.cost)
                {
                    retry = 0;
                    infoSA.cross_better++;
                    optSolution = tempSolution;
                    printf("[SA] 通过交叉获得最优解: %d， 当前时间 %ld ms\n", optSolution.cost,  (clock() - startTime_ns) / 1000);
                }
                solutions.push_back(tempSolution);
                sort(solutions.begin(), solutions.end(), LessSort);
                // for(auto i:solutions) printf("%d(%d)--", i.cost, i.age);
                // printf("\n");
                infoSA.operation[infoSA.last_operation] ++;
                infoSA.cross_good++;
            }
            else
                if(tempSolution.cost != _basicSolution.cost) retry++;
        }
        for (auto eachSolution = solutions.begin(); eachSolution != solutions.end(); ) 
        { 
            static int count = 0;
            if(eachSolution->age > MAX_AGE ){ 
                searchedSolution.insert(eachSolution->bits);
                if(count==0) printf("[SA] 正在脱离局部最优状态\n");
                eachSolution = solutions.erase(eachSolution); 
                count++; if(count > 10) count = 0;
            } 
            else{ 
                eachSolution++; 
            }
        }
        if(crossRate > 0.000001 && solutions.size() > popLimit) crossRate /= CROSS_RATE_DIVISION ;
        else if(crossRate <1) crossRate += CROSS_RATE_PLUE ;
        if(solutions.size() > popLimit ) {
            auto cut = unique(solutions.begin(), solutions.end(), uniqueSort);
            solutions.erase(cut,solutions.end());
            if(solutions.size() > popLimit) solutions.pop_back();
        }
        if(printClock < clock() - startTime_ns && 1)
        {
            printf("[SA] now: %d-%d \t", solutions[0].cost, solutions[solutions.size() - 1].cost);
            printf("mutate: %d/%d/%d \t", infoSA.mutate, infoSA.mutate_good, infoSA.mutate_better);
            printf("cross: %d/%d/%d \t", infoSA.cross, infoSA.cross_good, infoSA.cross_better);
            printf("operation: %d/%d/%d/%d/%d \t", infoSA.operation[1], infoSA.operation[2], infoSA.operation[3], infoSA.operation[4], infoSA.operation[5]);
            printf("Mincostflow run %d times, real run %d and cache %d.\n", infoMinCostFlow.runtimes + infoMinCostFlow.cache , infoMinCostFlow.runtimes,  infoMinCostFlow.cache);
            printf("retry times : %d \t", retry);
            printf("crossRate %.3f len: %d , \t used time %ld ms\n", crossRate , (int)solutions.size(), (clock() - startTime_ns) / 1000);
            printClock += PRINT_CLOCK_MS * 1000;
        }
    }
}

//你要完成的功能总入口
void deploy_server(char *topoOrigin[MAX_EDGE_NUM], int line_num, char *filename)
{
    long beginTime = clock();
    topo = topoOrigin;
    solutionPack optSolution;
    solutionPack basicSolution;
    string result;
    //1. 读取文件，并初始化变量:节点数、边数、消费节点数、服务器部署成本
    readFile();
    //2. 根据变量，建立ListDigraph, 节点编号： 0， 1， 2，，， (vetexNum - 1)
    initGraph();
    //3. 设置初解
    basicSolution.cost = consumerNum * costPerServer;
    for (int i = 0; i < vertexNum; i++)
    {
        if (consumerSet.count(i) == 0)
            basicSolution.solutionAlt.push_back(i);
        else{
            basicSolution.solution.push_back(i);
            basicSolution.bits[i] = 1;
        }
    }
    SA_SetBasicSolution(basicSolution);
    if (vertexNum > 280 && vertexNum < 320)
    {
        popLimit = 20;
        SA(optSolution, 15 * 1000 * 1000);
    }
    else if (vertexNum > 600 && vertexNum < 900)
    {
        popLimit = 20;
        SA(optSolution);
    }
    else
    {
        SA(optSolution);
    }
    cout << "最优解：" << optSolution.cost << ", 选址："<<endl;
    sort(optSolution.solution.begin(), optSolution.solution.end());
    for(auto i: optSolution.solution) printf("%d--", i);
    printf("\n");
    // //5. 重新运行最优解， 最后打印流量到答案文件中
    runMinCostFlow(optSolution, false);
    printMinFlowResult(result, superS, superT, consumerInfoForPrint);
    write_result(result.c_str(), filename);
    printf("[END] Used time %ld ms. \n", (clock() - beginTime) / 1000);
    printf("[END] Mincostflow run %d times, real run %d and cache %d.\n", infoMinCostFlow.runtimes + infoMinCostFlow.cache , infoMinCostFlow.runtimes,  infoMinCostFlow.cache);
    // networkSimplex->printGraph();
}
