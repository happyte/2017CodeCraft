#include "deploy.h"
#include "network_simplex.h"
#include "print_result.h"
#include "data.h"

shared_ptr<NetworkSimplex> networkSimplex;

void lowerHardwareLevel(solutionPack &solutionPack, string &result);

/*********************** 遗传算法相关 ***************************/

static int getCacheCnt = 0;
static int totalCount = 0;
static unordered_map<vector<bool>, map<vector<int>, solutionPack>> solutionCache;
ProblemScale ProbScale;

enum strategyType
{
    /*
    策略 0
    在该策略下，服务器上限设置为最小的服务器流量单价成本的等级，部署成本按照平均成本来算，全适应
    */
    strategy_mincost_average_level,
    /*
    策略 1
    在该策略下，服务器上限设置为最小的服务器流量单价成本的等级，部署成本按照各个服务器成本来算，全适应
    */
    strategy_mincost_level,

    strategy_adapt_mincost_level,
};



strategyType strategy = strategy_mincost_average_level;

int runMinCostFlow(solutionPack &solution, bool useCache)
{
    // useCache = false;
    vector<int> cachevector;
    const vector<bool> &chromosome = solution.solution;
    map<int, int> &capacityMap = solution.serverLevel;
    solution.serverFlow.clear();

    totalCount++;
    int serverCount = 0;
    //4. 设置超级源点边的容量信息,cost依旧为0
    for (int index = 0; index < (int)(chromosome.size()); index++)
    {
        if (chromosome[index])
        {
            serverCount++;
            auto tmp = capacityMap.find(index);
            if (tmp == capacityMap.end())
            {
                cout << "没有对应key的value" << endl;
                break;
            }
            int levelIndex = tmp->second;
            int arc = superS_arc[index][levelIndex];
            networkSimplex->stCost(0, arc);
        }
    }

    //5. 针对解： 调用方法求出最小费用
    NetworkSimplex::ProblemType ret;
    if (totalCount < 10)
        ret = networkSimplex->run();
    else
    {
        networkSimplex->fixPotential();
        ret = networkSimplex->directRun();
    }
    int cost, fitness;
    if (ret == NetworkSimplex::ProblemType::OPTIMAL)
    {
        //计算cost需要考虑不一样的服务器不一样的硬件成本 + 部署费
        int serverDeployCost = 0;
        int serverHardwareCost = 0;
        for (int i = 0; i < vertexNum; i++)
        {
            if (chromosome[i])
            {
                int deployCost = deployCostEachNode[i];
                serverDeployCost += deployCost;
                int flow = superS_arc_flow(i);
                pair<int, int> hdInfo = getHardwareLevelInfoByFlow(flow);
                int hardwareId = hdInfo.first;
                int hardwareCost = hdInfo.second;
                serverHardwareCost += hardwareCost;

                //更新capacityMap
                capacityMap[i] = hardwareId;
                solution.serverFlow[i] = flow;
            }
        }
        cost = (*networkSimplex).totalCost() + serverDeployCost + serverHardwareCost;
        fitness = MAX_COST - cost;
    }
    else
    {
        cost = MAX_COST;
        fitness = 0;
    }
    //恢复添加的超级源点的边的容量为0:恢复图
    for (int index = 0; index < (int)(chromosome.size()); index++)
    {
        for (uint j = 0; j < hardwareLevel.size(); j++)
        {
            if (chromosome[index])
            {
                int arc = superS_arc[index][j];
                networkSimplex->stCost(INT_MAX, arc);
            }
        }
    }
    //记录到缓存中
    solution.fitness = fitness;
    return fitness;
}

static void createInitBasicSolution(solutionPack &basicSolution, int vertexNumber)
{

    //Small
    //    vector<int> mySolution = {3, 14, 36, 69, 103, 129, 155}; //48849 -> 48345
    //    vector<int> mySolution = {3, 7, 14, 35, 36, 69, 129, 155}; //48795 -> 48290
    //Middle
    //    vector<int> mySolution = {34,45,51,75,159,173,200,212,221,228,246,257,277,286,287,299};//90216 -> 89238
    //    vector<int> mySolution = {22,45,51,61,75,93,159,173,200,212,221,228,246,277,286,299};//90523 -> 89245
    //    vector<int> mySolution = {3,34,45,51,93,157,159,173,196,212,221,246,275,277,286,299};//91384 -> 89745
    //    vector<int> mySolution = {2,34,45,51,95,141,157,158,173,200,212,221,229,246,275,286,299};//89816 -> 89281
    //Big
    //237033 -> 234905
    //    vector<int> mySolution = {14,19,22,62,72,94,99,128,142,154,182,184,192,194,226,245,257,265,279,320,335,351,370,381,386,413,445,481,488,508,511,541,543,547,610,620,641,673,679,690,705,715,745,768,772,786,787,796};
    //235698 -> 233228
    //    vector<int> mySolution = {14,19,22,32,34,54,62,68,94,99,108,128,147,154,182,184,192,194,215,223,226,243,257,265,301,320,335,339,351,370,382,413,425,445,466,481,486,508,530,541,543,547,579,610,620,641,691,705,745,768};
    //235017 -> 233162
    //    vector<int> mySolution = {14,15,19,22,30,32,54,62,68,94,99,142,147,182,184,192,215,245,251,257,265,279,282,320,339,351,370,383,397,413,415,425,445,466,481,511,541,543,579,610,626,637,639,641,672,691,715,768,772};
    //745754 -> 720648
    //    vector<int> mySolution = {3,7,8,9,10,11,17,19,22,23,25,27,37,38,39,40,41,43,44,45,52,53,54,56,58,59,62,64,73,74,75,80,92,95,99,102,109,112,114,115,117,128,129,132,133,135,136,149,150,151,152,154,156,157,160,163,165,166,167,169,174,177,178,182,183,189,195,197,199,200,206,207,208,210,211,212,213,220,221,222,223,224,231,232,234,235,236,238,239,241,243,244,245,246,247,248,249,256,257,259,260,262,263,265,266,268,269,271,273,280,281,282,283,285,288,289,290,291,304,305,308,312,313,314,324,325,326,327,328,329,332,333,340,341,342,344,346,347,349,353,354,357,364,365,367,375,377,389,391,392,393,394,396,397,399,400,403,404,405,408,412,413,415,416,418,420,421,422,423,430,431,432,433,438,439,448,449,450,451,453,454,456,466,471,473,474,477,478,481,482,483,489,491,492,494,496,497,502,511,512,513,516,517,518,519,521,522,527,529,530,533,534,540,541,543,549,550,552,554,556,557,558,559,563,565,566,569,570,572,580,582,584,585,586,587,588,589,591,592,593,595,596,604,606,608,609,610,611,612,613,614,615,616,617,619,622,627,629,631,634,639,640,644,645,647,655,658,659,661,662,663,665,668,669,674,675,678,679,683,684,685,686,689,693,695,696,698,699,709,710,711,712,714,715,721,722,726,727,728,730,741,742,746,748,749,755,759,760,764,765,766,773,776,781,782,788,796,797,799};

    //232 vs 2337
    //vector<int> mySolution = {15, 626, 371, 20, 370, 191, 770, 690, 215, 361, 276, 486, 579, 1, 445, 771, 750, 562, 653, 603, 507, 257, 679, 413, 182, 365, 62, 19, 543, 245, 689, 519, 610, 639, 282, 22, 256, 715, 530, 533, 658, 541, 552, 474, 305, 563, 389, 439};

    //2330 vs 2334
    //    vector<int> mySolution = {626, 21, 20, 690, 786, 301, 486, 772, 447, 579, 1, 636, 625, 464, 536, 194, 653, 0, 603, 575, 54, 257, 265, 679, 92, 397, 99, 62, 19, 543, 245, 689, 519, 610, 796, 282, 256, 25, 530, 367, 541, 223, 349, 669, 474, 305, 117, 439,};

    //231775
    //    vector<int> mySolution = {14,15,19,22,30,32,54,62,68,92,94,140,147,182,192,215,223,227,245,256,257,265,282,302,334,339,349,351,370,383,387,389,397,425,461,481,482,511,536,541,543,579,610,620,626,639,641,673,679,691,768,786};

    //    使用测试的初始解
    //    for (int serverNodeId : mySolution){
    //        population[0].solution[serverNodeId] = true;
    //        population[0].serverLevel.insert(make_pair(serverNodeId, maxServerLevel));
    //        cout << maxServerLevel << endl;
    //    }

    //2.设置第一个初始解

    basicSolution.solution.resize(vertexNumber);
    for (int i = 0; i < (int)(basicSolution.solution.size()); i++)
    {
        basicSolution.solution[i] = false;
    }
    basicSolution.serverLevel.clear();
    //使用直连解
    for (vector<int> con : consumerInfo)
    {
        basicSolution.solution[con[1]] = true;
        basicSolution.serverLevel.insert(make_pair(con[1], maxServerLevel));
    }

    basicSolution.fitness = runMinCostFlow(basicSolution, false);
   
    //3.保留直连解
    directConnSolution = basicSolution;
}

//************************************  [START]核心的三种Mutate  ************************************

int getRandomNeighbor(int searchNode, int k)
{
    if (k == 1)
    {
        int pos = rand() % (int)edgesMap[searchNode].size();
        return edgesMap[searchNode][pos];
    }
    else
    {
        vector<int> searchRang;
        vector<int> nodes = edgesMap[searchNode];
        searchRang.insert(searchRang.end(), nodes.begin(), nodes.end());
        for (int i = 0; i < (int)nodes.size(); ++i)
        {
            vector<int> neighbor = edgesMap[nodes[i]];
            searchRang.insert(searchRang.end(), neighbor.begin(), neighbor.end());
        }
        int pos = rand() % (int)searchRang.size();
        return searchRang[pos];
    }
}

static void addServer(solutionPack &solution)
{
    //add server
    int position = rand() % vertexNum;
    int realPosition;
    for (int i = 0; i < vertexNum; i++)
    {
        realPosition = (position + i) % vertexNum;
        if (solution.solution[realPosition] != true)
        {
            solution.solution[realPosition] = true;
            solution.serverLevel.insert(make_pair(realPosition, maxServerLevel));
            break;
        }
    }
    //新增的服务器档位，默认为最大
}
static void delServer(solutionPack &solution)
{
    //delete server
    int position = rand() % vertexNum;
    int realPosition;
    for (int i = 0; i < vertexNum; i++)
    {
        realPosition = (position + i) % vertexNum;
        if (solution.solution[realPosition] == true)
        {
            solution.solution[realPosition] = false;
            solution.serverLevel.erase(realPosition);
            break;
        }
    }
    //删除对应的档位
}

static void chgServer(solutionPack &solution, int k = 1)
{
    //1. 先找到一台服务器
    int position1 = rand() % vertexNum; //server
    for (int i = 0; i < vertexNum; i++)
    {
        position1 = (position1 + i) % vertexNum;
        if (solution.solution[position1] == true)
        {
            break;
        }
    }
    //2. 随机选择一台临近的非服务器
    int position2 = getRandomNeighbor(position1, k);
    solution.solution[position1] = false; //删除一台
    solution.solution[position2] = true;  //增加一台
    //修改对应的档位
    solution.serverLevel.erase(position1);
    solution.serverLevel.insert(make_pair(position2, maxServerLevel));
}

//************************************  [END]核心的三种Mutate  ************************************

static solutionPack SA_getANewSolution(const solutionPack &originalSolution)
{
    float random = rand01();
    solutionPack newSolution = originalSolution;
    double totalWays = 3.0;
    int serverCount = 0;
    if (random < 1.0 / totalWays)
    {
        addServer(newSolution);
    }
    else if (random < 2.0 / totalWays)
    {
        delServer(newSolution);
    }
    else if (random < 3.0 / totalWays)
    {
        chgServer(newSolution, 1);
    }
    // 这里每次都把newSolution的serverLevel设置为最大那
    newSolution.serverLevel.clear();
    newSolution.serverFlow.clear();
    newSolution.fitness = 0;

    for (int i = 0; i < (int)newSolution.solution.size(); ++i)
    {
        if (newSolution.solution[i])
        {
            serverCount++;
        }
    }

    int setLevel = maxServerLevel;

    if (strategy == strategy_mincost_average_level)
    {
        setLevel = minCostServerLevel;
    }
    if (strategy == strategy_adapt_mincost_level)
    {
        setLevel = adapt_mincost_level;
    }

    if (strategy != strategy_mincost_level)
    {
        for (int i = 0; i < (int)newSolution.solution.size(); ++i)
        {
            if (newSolution.solution[i])
            {
                newSolution.serverLevel[i] = setLevel;
            }
        }
    }
    else
    {
        for (int i = 0; i < (int)newSolution.solution.size(); ++i)
        {
            if (newSolution.solution[i])
            {
                newSolution.serverLevel[i] = max(minCostServerLevelVector[i], 0);
            }
        }
    }

    runMinCostFlow(newSolution, true);
    return newSolution;
}


bool Accept(const solutionPack &newSolution, const solutionPack &originalSolu, double T_cur);
solutionPack SA(const solutionPack &basicSolution, const double T, long timeLimit, bool UseLowerHw, long timeUseLowerHw = 90*1000*1000){
    double SPEED ;
    double T_cur = T;
    SPEED = pow(1.0/T, 1.0/timeLimit);
    long SA_beginTime = clock();
    long printClock = 0;
    solutionPack searchSolution = basicSolution;
    while ((clock() - SA_beginTime) < timeLimit )
    {
        solutionPack newSolution = SA_getANewSolution(searchSolution);
        if (UseLowerHw)
        {
            if (newSolution.fitness > 0)
            {
                lowerHardwareLevel(newSolution, result);
            }
        }
        if (Accept(newSolution, searchSolution, T_cur))
        {
            searchSolution = newSolution;
        }
        long runTime = (clock() - SA_beginTime) ;
        T_cur = T * pow((double)SPEED, (double)runTime);
        globalCount++;
        if (printClock < runTime){
            printClock+=1000*1000;
            printf("最优：" RED "%d \t" NONE
                   " 当前：%d 本次：%d \t 温度：%lf \t 时间：%ld ms 总时间：%ld ms\n",
                   MAX_COST - optSolution.fitness, MAX_COST - searchSolution.fitness, MAX_COST - newSolution.fitness, T_cur, runTime/1000, (clock() - beginTime) / 1000);
        }
        if (runTime >= timeUseLowerHw)
        {
            UseLowerHw = true;
        }
    }
    return searchSolution;
}


void deploy_server(char *topoOrigin[MAX_EDGE_NUM], int line_num, char *filename)
{
    srand((unsigned int)time(NULL));
    beginTime = clock();
    topo = topoOrigin;
    //1. 读取文件，并初始化变量:节点数、边数、消费节点数、服务器部署成本
    readFile();
    getMaxCostAndMaxServCap();
    
    //2. 根据变量，建立graph
    initGraph(networkSimplex);
    globalCount = 0;

    long max_run_time = 87 * 1000 * 1000;
    long adapt_time = 5 * 1000 * 1000;
    int timeUseLowerHw ;
    double T;
    int minCostServerLevelsLen;         //预选档个数

    if (vertexNum >= 1000)
    { //大规模

        ProbScale = Big;
        T = 80;
        timeUseLowerHw = 20* 1000 * 1000;
        strategy = strategy_adapt_mincost_level;
        minCostServerLevelsLen = 3;
    }
    else 
    { //中规模
        ProbScale = Middle;
        T = 40; //T越小，接受差解的概率就越小
        timeUseLowerHw = 10* 1000 * 1000;
        strategy = strategy_adapt_mincost_level;
        minCostServerLevelsLen = 3;
    }
    getMinCostServerLevel(minCostServerLevelsLen);

    solutionPack basicSolution;
    //创建初始种群
    createInitBasicSolution(basicSolution, vertexNum);
    optSolution = basicSolution;
    //单测试降档
    //    lowerHardwareLevel(basicSolution, result);    
    //    exit(1);

    //预选档 评估压档
    map<int, int> adapt_cost_level;
    map<int, solutionPack> adapt_level_solution;
    if(strategy == strategy_adapt_mincost_level && minCostServerLevelsLen!=0){
        for (auto i : minCostServerLevels)
        {
            printf("尝试设置最大档位为%d\n", i.second);
            adapt_mincost_level = i.second;
            solutionPack endSolution = SA(basicSolution,T, adapt_time, false);
            printf("%d档位最终费用为%d\n",i.second, MAX_COST - endSolution.fitness);
            adapt_cost_level[MAX_COST - endSolution.fitness] = i.second;
            adapt_level_solution[i.second] = endSolution;
        }
        for( auto i: adapt_cost_level){
            printf("测试解费用%d, 其档位设置为%d\n", i.first, i.second);
        }
        adapt_mincost_level = adapt_cost_level.begin()->second;
        basicSolution = adapt_level_solution[adapt_mincost_level];
        timeUseLowerHw -= adapt_time;
        printf("最终选择：%d，初解费用：%d\n", adapt_mincost_level, MAX_COST - basicSolution.fitness);
    }

    printf("开始运行\n");
    if(ProbScale == Big ){
        SA(basicSolution, T, max_run_time - minCostServerLevels.size() * adapt_time, false ,timeUseLowerHw);
    }
    else{
        SA(basicSolution, T, max_run_time - minCostServerLevels.size() * adapt_time, false ,timeUseLowerHw);
    }

    //输出到文件
    runMinCostFlow(optSolution, false);
    printMinFlowResult(result, vertexNum, vertexNum + 1, consumerInfoForPrint);
    write_result(result.c_str(), filename);
    //打印
    cout << endl;
    cout << "最优解：" << MAX_COST - optSolution.fitness;
    cout << "\t\tGlobalCount =" << globalCount;
    cout << "\tTime used: " << (clock() - beginTime) / 1000 << "ms" << endl;
    cout << "最小费用流共运行：" << totalCount << "\tcache命中：" << getCacheCnt << endl;
}

bool Accept(const solutionPack &newSolution, const solutionPack &originalSolu, double T_cur)
{
    //更新最优解
    if (newSolution.fitness == 0)
        return false;
    if (newSolution.fitness > optSolution.fitness)
    {
        optSolution = newSolution;
        globalCount = 0;
    }
    if (newSolution.fitness >= originalSolu.fitness)
    {
        return true;
    }
    // int dCost = 0;
    double Pt;
    if (ProbScale == Big)
    {
        // Pt = 0;
        Pt = exp((newSolution.fitness - originalSolu.fitness) / (T_cur));
    }
    else
    {
        // Pt = 0;
        Pt = exp((newSolution.fitness - originalSolu.fitness) / (T_cur));
    }
    // dCost = newSolution.fitness - originalSolu.fitness;

    // cout << "【Pt】" << Pt << "\t dc=" << dCost << "\n"
    //      << endl;
    if (rand01() <= Pt)
    {
        printf("Accept\n");
        return true;
    }
    else
    {
        return false;
    }
}

bool BiggerSort(const pair<int, int> &comp1, const pair<int, int> &comp2) { return (comp1.second > comp2.second); } //从大到小
//保存： <节点：档位> startIndex从0开始
bool updateCapacity2(solutionPack &solution, int startIndex)
{
    //找到流量单价最高的，把它的服务器降档
    //0档先不降
    map<int, int> &nodeCapacity = solution.serverLevel;
    vector<pair<int, double>> compition;
    for (auto ite = nodeCapacity.begin(); ite != nodeCapacity.end(); ite++)
    {
        int nodeId = ite->first;
        int level = ite->second;
        if (level == 0)
            continue;

        //得到可以转移的流量
        int actualFlow = solution.serverFlow.at(nodeId);
        int smallerCapacity = hardwareLevel[level - 1].at(1);
        int diff = actualFlow - smallerCapacity;
        int currLevelHwCost = hardwareLevel[level].at(2);
        int smallerLevelHwCost = hardwareLevel[level - 1].at(2);

        double costPerFlow = (double)(currLevelHwCost - smallerLevelHwCost) / (double)diff;
        if (diff < 0)
            cout << "smaller than 0" << endl;

        compition.push_back(make_pair(nodeId, costPerFlow));
    }
    sort(compition.begin(), compition.end(), BiggerSort);
    //每次只变换一个
    if (startIndex >= (int)compition.size())
        return false;
    int changeNodeId = compition[startIndex].first;
    auto ite = nodeCapacity.find(changeNodeId);
    ite->second--;
    return true;
}

//更新nodeCapacity，并更新optSolution
void lowerHardwareLevel(solutionPack &solution, string &result)
{
    int startIndex = 0;
    solutionPack search_solution = solution;
    // cout << "search" << endl;
    while (1)
    {
        // cout << startIndex << "_" ;
        //从当前最优重根的基础上进行搜索
        if (!updateCapacity2(search_solution, startIndex++))
        {
            cout << "已经全部搜过了" << endl;
            break;
        }
        int newFitness = runMinCostFlow(search_solution, false);
        //只要新解是无解的，那么就停止服务器降档了
        if (newFitness == 0)
        {
            // cout << "搜到无解，退出" << endl;
            break;
        }
        if (newFitness > solution.fitness)
        { //只要比原来的解好，就替换原来的解(其实也只是替换fitness、serverLevel)： 贪心搜索重根
            solution = search_solution;
            startIndex = 0;
            //            cout << "找到更好重根：" << MAX_COST - newFitness << "\tUsed Time:" << (clock() - beginTime) / 1000 << endl;
        }
        //更新最优解
        if (solution.fitness > optSolution.fitness)
        {
            optSolution = solution;
            printf("[降档]\t 最优解：%d \t 时间:%ld \n", MAX_COST - optSolution.fitness, (clock() - beginTime) / 1000);
            globalCount = 0;
        }
        
    }
}
