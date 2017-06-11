#include "general_head.h"
#include "data.h"
#include "utils.h"
#include "deploy.h"
#include "SA.h"
#include "analysis.h"
#include <sys/time.h>
#include <cassert>

static SA_config _config;

/************** 计算轮利润 *****************/
void calcRoundProfit(solutionPack &solution, bool updateLevel, bool updateNeed)
{
    int flowCost = runMinCostFlow(solution.serverLevel, solution.consumerNeedMap);
    
    //如果费用流无解, 则直接返回
    if (flowCost == -1 && updateNeed == false)
    {
        solution.feasible = false;
        return;
    }
    else if(flowCost == -1 && updateNeed == true){
        auto newConsumerNeedMap = solution.consumerNeedMap;
        for(auto i:solution.consumerNeedMap){
            int id = i.first;
            int need = i.second;
            if(superT_arc_flow(id) != need){
                newConsumerNeedMap.erase(id);
            }
        }
        solution.consumerNeedMap = newConsumerNeedMap;
        flowCost = runMinCostFlow(solution.serverLevel, solution.consumerNeedMap);
        assert(flowCost != -1);
        solution.feasible = true;
    }
    else {
        solution.feasible = true;
    }

    solution.flowCost = flowCost;

     // 更新流量与服务器档位信息
    for (int i = 0; i < vertexNum; i++)
    {
        if (solution.solution.at(i))
        {
            int nodeid = i;
            int flow = superS_arc_flow(nodeid);
            //清除流量为0的服务器
            if (flow == 0 && updateLevel && _config.sale_server)
            {
                solution.solution[nodeid] = 0;
                solution.serverLevel.erase(nodeid);
                solution.serverFlow.erase(nodeid);
                continue;
            }

            else if(flow == 0){
                solution.zeroFlowServer.insert(nodeid);
            }
            else if(flow != 0){
                solution.zeroFlowServer.erase(nodeid);
            }
            //暂时不更新档位
            //档位的变动引起复杂的买卖问题

            // pair<int, int> hdInfo = getHardwareLevelInfoByFlow(flow);
            // int hardwareId = hdInfo.first;
            // if(updateLevel)
            //     solution.serverLevel[nodeid] = hardwareId;
            solution.serverFlow[nodeid] = superS_arc_flow(nodeid);
        }
    }

    int consumerFee = 0;
    for(auto i: solution.consumerNeedMap){
        int id = i.first;
        if(competitorConsumerHistorySet.count(id) == 1){
            consumerFee += fee/2;
        }
        else {
            consumerFee += fee;
        }
    }
    
    solution.roundProfit = consumerFee - flowCost;
}

void fixRoundProfit(solutionPack &solution){
    //消费者变少了, 修正 income
    int consumerFee = totalEarnConsumerNum * fee;
    solution.roundProfit = consumerFee - solution.flowCost;    
}


/************ 计算服务器变动的花费 *************/

int calcServerChangeCost(solutionPack & solution, const solutionPack &lastSolution, bool printInfo = false){
    if(printInfo)
    {
        printf("服务器变更情况以及变卖情况:\n");
        printf("上一次的服务器：\n");
        for(int i=0;i<vertexNum;i++){
            if(lastSolution.solution.at(i)) printf("%d ", i);
        }
        printf("\n");
        printf("本次服务器:\n");
        for(int i=0;i<vertexNum;i++){
            if(solution.solution.at(i)) printf("%d ", i);
        }
        printf("\n");
    }
    int chgCost = 0;
    //对比服务器
    
    for (int i = 0; i < vertexNum; i++)
    {
        int level;
        int cost;
        int soldCost = 0;
        int buyLevel;
        int buyCost = 0;
        int deployCost = 0;
        if(lastSolution.solution.at(i)){
            //上次部署了
            //本次部署了
            if(solution.solution.at(i)){
                //一样的结果
                if(lastSolution.serverLevel.at(i) == solution.serverLevel.at(i)){
                    //没有额外花费
                    solution.serverDeployTime[i] = lastSolution.serverDeployTime.at(i);
                    if(printInfo){
                        printf("继承 节点%d部署时间为%d, 保持不变\n", i, solution.serverDeployTime[i]);
                    }
                }
                //不一样
                else{
                    level = lastSolution.serverLevel.at(i);
                    cost = hardwareLevel.at(level).at(2);
                    int usedtime = deployClock - lastSolution.serverDeployTime.at(i);
                    soldCost = cost * 0.8 * (600.0 - usedtime) / 600.0;
                    buyLevel = solution.serverLevel.at(i);
                    buyCost = hardwareLevel.at(buyLevel).at(2);
                    solution.serverDeployTime[i] = deployClock;
                    if(printInfo){
                        printf("买卖 节点%d部署时间为%d, 当前时间为%d, 用时%d, 档位由%d变为%d, 原价%d, 卖掉%d, 买入%d \n",
                               i, lastSolution.serverDeployTime.at(i), deployClock, usedtime, level, buyLevel, cost, soldCost, buyCost);
                    }
                }
            }
            //本次没有部署
            else{
                assert(_config.sale_server == true);
                level = lastSolution.serverLevel.at(i);
                cost = hardwareLevel.at(level).at(2);
                int usedtime = deployClock - lastSolution.serverDeployTime.at(i);
                soldCost = cost * 0.8 * (600.0 - usedtime) / 600.0;
                if (printInfo)
                {
                    printf("卖掉 节点%d部署时间为%d, 当前时间%d, 用时%d, 档位%d 原价%d, 卖掉%d \n",
                        i, lastSolution.serverDeployTime.at(i), deployClock, usedtime, level, cost, soldCost);
                }   
            }
        }
        else{
            //上次没有部署
            //本次部署了
            if(solution.solution.at(i)){
                buyLevel = solution.serverLevel.at(i);
                buyCost = hardwareLevel.at(buyLevel).at(2);
                deployCost = deployCostEachNode.at(i);
                solution.serverDeployTime[i] = deployClock;
                if (printInfo)
                {
                    printf("买入 节点%d部署时间为%d, 档位%d 买入%d, 部署%d \n",
                           i, deployClock, buyLevel, buyCost, deployCost);
                }
            }

        }
        chgCost = chgCost + buyCost + deployCost - soldCost;
    }
    return chgCost;
}

/************** 目标函数 *******************/
//目标是每一轮的利润最大化
//计算如下几项内容:
/*
solution.flowCost
solution.serverFlow
solution.roundProfit
solution.totalProfit
*/


void objectiveFunction(solutionPack &solution, const solutionPack &lastSolution, bool updateLevel = true, bool updateNeed = false)
{
    //计算轮净利润(不含部署费)
    calcRoundProfit(solution, updateLevel, updateNeed);
    if(solution.feasible == false) return;

    //计算服务器变动费用
    //每台服务器, 第一次部署的时候, 因为总会卖出, 卖出的时候部署费不折旧, 所以一部署, 总的金钱就减少了
    int chgCost = calcServerChangeCost(solution, lastSolution);

    //计算本轮的总净利润(含部署费)
    //本轮增加的轮净利润(不含部署费) * 剩余时间 就是 本轮改变后的总净利润(不含部署费)
    //本轮改变后的总净利润(不含部署费) - 部署费用 = 本轮改变后的总净利润(含部署费)
    solution.totalProfit = solution.roundProfit * (600 - deployClock) / 10 - chgCost;

    //计算消费者的奖励
    solution.fixProfit = 0;

    for (auto ite : competitorConsumerSet){ 
        int id = ite; 
        if (solution.consumerNeedMap.count(id) == 0){ 
            solution.fixProfit -= fee * (600 - deployClock) / 10; 
        } 
    } 

    solution.fixTotalProfit = solution.totalProfit + solution.fixProfit;
}

/*************** 约束函数 ****************/
//约束函数是服务器的变动以及档位的调整不能导致破产
/*
计算如下几项内容
solution.serverChangeCost
solution.serverDeployTime
*/




void constraintFunction(solutionPack & solution, const solutionPack &lastSolution, bool printInfo = false)
{
    //计算部署费用
    //为了使得净利润最大化, 我们需要多部署服务器, 部署服务器只要保证当前不破产就行了,在不破产的前提下部署尽可能多的服务器
    //如果部署服务器后还有钱, 只要净利润大于0, 那么我们就不会破产

    int chgCost = calcServerChangeCost(solution, lastSolution, printInfo);
    solution.serverChangeCost = chgCost;
    
    // 检查服务器花费是否超过了限制
    if(_config.check_myMoney == false) return;
    if(solution.serverChangeCost + solution.flowCost >= myMoney) {
        solution.feasible = false;
        return;
    }
    else {
        solution.feasible = true;
    }
}

void calcSolution(solutionPack &solution, const solutionPack &lastSolution, bool printInfo = false, bool updateLevel = true, bool updateNeed = false)
{
    //计算目标函数
    //也就是本轮净利润
    objectiveFunction(solution, lastSolution, updateLevel, updateNeed);
    if(solution.feasible == false) return;

    //计算约束函数
    //也就是服务器部署变动导致的花费
    constraintFunction(solution, lastSolution, printInfo);

    //如果方案合法, 贪心搜索该方案的最佳档位
    // lowerHardwareLevel(solution);
    
}

/******************** SA局部搜索 *****************/

static int getNeighborUnSelectedConsumer(const solutionPack &solution, const int consumerId)
{
    bool hasChecked[vertexNum];
    memset(hasChecked, false, vertexNum * sizeof(bool));
    queue<int> queue;
    vector<int> neighbors = edgesMap[consumerId];
    hasChecked[consumerId] = true;
    for (auto ite : neighbors){
        queue.push(ite);
    }

    int searchNodeId;
    while( !queue.empty() ){
        searchNodeId = queue.front();
        queue.pop();

        //标记搜索过
        hasChecked[searchNodeId] = true;
        //首先把搜索点相邻的未搜索过的节点入队
        vector<int> neighbors = edgesMap[searchNodeId];
        for (auto ite : neighbors){
            if (hasChecked[ite] == false){
                queue.push(ite);
            }
        }

        //如果找到符合条件的就退出: 是消费节点，但是还未选择
        if ((solution.consumerNeedMap.count(searchNodeId) == 0) && (consumerAnalysisMap.count(searchNodeId) == 1)){
            // printf("【搜索消费节点】%d \t 需求：%d \t 邻居消费节点：%d \t 邻居需求：%d\n",
                // consumerId, consumerAnalysisMap[consumerId], searchNodeId, consumerAnalysisMap[searchNodeId]);
            return searchNodeId;
        }

    }
    return -1;
}

static void addServer(solutionPack &solution)
{
    //add server
    int position = rand() % vertexNum;
    int realPosition;
    for (int i = 0; i < vertexNum; i++)
    {
        realPosition = (position + i) % vertexNum;
        // if(realPosition == 0) continue;
        if (solution.solution[realPosition] != true)
        {
            solution.solution[realPosition] = true;
            int addConsumerNum = rand() % 3 + 1;
            for(int i=0; i<addConsumerNum; i++)
            {
                if(_config.change_consumer == false){
                    break;
                }
                int neighborUnSelectedConsumerId = getNeighborUnSelectedConsumer(solution, realPosition);
                // int neighborUnSelectedConsumerId = getRandomConsumer(realPosition);
                if (neighborUnSelectedConsumerId == -1)
                {
                    cout << "Error : not found consumer node" << endl;
                    break;
                }

                //添加这个消费者 
                solution.consumerNeedMap[neighborUnSelectedConsumerId] = consumerAnalysisMap[neighborUnSelectedConsumerId];
            }
            break;
        }
    }
}
static void delServer(solutionPack &solution, const solutionPack &lastSolution)
{
    //delete server
    int position = rand() % vertexNum;
    int realPosition;
    for (int i = 0; i < vertexNum; i++)
    {
        realPosition = (position + i) % vertexNum;
        if (solution.solution[realPosition] == true)
        {
            if(_config.sale_server == false && lastSolution.solution.at(realPosition) == 1){
                continue;
            }
            solution.solution[realPosition] = false;
            // int addConsumerNum = rand() % 3 + 1;
            // for(int i=0; i<addConsumerNum; i++)
            // {
            //     int neighborUnSelectedConsumerId = getNeighborUnSelectedConsumer(solution, realPosition);
            //     if (neighborUnSelectedConsumerId == -1)
            //     {
            //         cout << "Error : not found consumer node" << endl;
            //         break;
            //     }

            //     //删除这个消费者
            //     solution.consumerNeedMap.erase(neighborUnSelectedConsumerId);
            // }
            break;
        }
    }
}

int getRandomNeighbor(int searchNode)
{
    int pos = rand() % (int)edgesMap[searchNode].size();
    return edgesMap[searchNode][pos];
}

static void chgServer(solutionPack &solution, const solutionPack &lastSolution)
{
    //1. 先找到一台服务器
    int position1 = rand() % vertexNum; //server
    for (int i = 0; i < vertexNum; i++)
    {
        position1 = (position1 + i) % vertexNum;
        if (solution.solution[position1] == true)
        {
            if (_config.sale_server == false && lastSolution.solution.at(position1) == 1)
            {
                continue;
            }
            //2. 随机选择一台临近的非服务器
            int position2 = getRandomNeighbor(position1);
            solution.solution[position1] = false; //删除一台
            solution.solution[position2] = true;  //增加一台
            break;
        }
    }
}

static void addConsumer(solutionPack &solution)
{
    //先找到一个消费者
    int position = rand() % vertexNum;
    int realPosition;
    for (int i = 0; i < vertexNum; i++)
    {
        realPosition = (position + i) % vertexNum;
        //这个节点是消费者, 并且没有被选中
        if( consumerAnalysisMap.count(realPosition) == 1 && solution.consumerNeedMap.count(realPosition) == 0 )
        {
            solution.consumerNeedMap[realPosition] = consumerAnalysisMap[realPosition];
            break;
        }
    }
}

static void addConsumer(solutionPack &solution);

static void addNeighborConsumer(solutionPack &solution)
{
    if(solution.consumerNeedMap.size() == 0){
        addConsumer(solution);
        return;
    }

    //先随机一个已经选择的消费者
    int position = rand() % vertexNum;
    int realPosition;
    for (int i = 0; i < vertexNum; ++i) {
        realPosition = (position + i) % vertexNum;
        if (solution.consumerNeedMap.count(realPosition) == 1){
            //找到他邻域中的一个当前未选择的消费者
            int neighborUnSelectedConsumerId = getNeighborUnSelectedConsumer(solution, realPosition);
            if (neighborUnSelectedConsumerId == -1)
            {
                cout << "Error : not found consumer node" << endl;
            }

            //添加这个消费者
            solution.consumerNeedMap[neighborUnSelectedConsumerId] = consumerAnalysisMap[neighborUnSelectedConsumerId];
            if (rand01() > 0.5)
            {
                solution.solution[neighborUnSelectedConsumerId] = true;
            }
            break;
        }
    }

}


static void delConsumer(solutionPack &solution, const solutionPack &basicSolution)
{
    //先找到一个消费者
    int position = rand() % vertexNum;
    int realPosition;
    for (int i = 0; i < vertexNum; i++)
    {
        realPosition = (position + i) % vertexNum;
        //这个消费者节点是选中的消费者节点
        if( solution.consumerNeedMap.count(realPosition) == 1 )
        {
            //不删除基础解中的消费者
            if(basicSolution.consumerNeedMap.count(realPosition) == 1){
                continue;
            }
            solution.consumerNeedMap.erase(realPosition);
            break;
        }
    }
}


/************** 基于已有的解, 获取一个新解 ****************/

/*
计算如下几项内容
solution.solution
solution.serverLevel
solution.consumerNeedMap
*/
static solutionPack SA_getANewSolution(const solutionPack &originalSolution, const solutionPack &lastSolution, const solutionPack &basicSolution)
{
    float random;
    solutionPack newSolution;
    double totalWays;
    int serverCount;

    //产生初始解的时候 可以删除消费者
    // if(deployClock == 0){
        random = rand01();
        newSolution = originalSolution;
        totalWays = 5.0;
        serverCount = 0;

        if(_config.change_consumer == false) totalWays = 3.0;

        //局部搜索
        if (random < 1.0 / totalWays)
        {
            addServer(newSolution);
        }
        else if (random < 2.0 / totalWays)
        {
            delServer(newSolution, lastSolution);
        }
        else if (random < 3.0 / totalWays)
        {
            chgServer(newSolution, lastSolution);
        }
        else if(random < 4.0 / totalWays)
        {
            if(_config.change_consumer == true) {
                addConsumer(newSolution);
                // addNeighborConsumer(newSolution);
            }
        }
        else{
            if(_config.change_consumer == true)
                delConsumer(newSolution, basicSolution);
        }
    // }else{//如果不是产生初始解的话，那么是不能删除消费者的
    //     newSolution = originalSolution;
    //     serverCount = 0;

    //     if(_config.change_consumer == false) totalWays = 3.0;

    //     double mutateServ = rand01();
    //     if (mutateServ < 0.6){
    //         totalWays = 3;
    //         random = rand01();
    //         //局部搜索
    //         if (random < 1.0 / totalWays)
    //         {
    //             addServer(newSolution);
    //         }
    //         else if (random < 2.0 / totalWays)
    //         {
    //             delServer(newSolution, lastSolution);
    //         }
    //         else if (random < 3.0 / totalWays)
    //         {
    //             chgServer(newSolution, lastSolution);
    //         }
    //     }else{

    //         if(_config.change_consumer == true) {
    //             addConsumer(newSolution);
    //             // addNeighborConsumer(newSolution);
    //         }
    //     }
    // }


    newSolution.serverLevel.clear();
    newSolution.serverFlow.clear();
    newSolution.serverDeployTime.clear();
    newSolution.zeroFlowServer.clear();

    //计算服务器数量
    for (int i = 0; i < (int)newSolution.solution.size(); ++i)
    {
        if (newSolution.solution[i])
        {
            serverCount++;
        }
    }
    //设置初始档位
    // int setLevel = minCostServerLevel;
    for (int i = 0; i < (int)newSolution.solution.size(); ++i)
    {
        if (newSolution.solution[i])
        {
            int setLevel = serverMaxLevel.at(i);
            // int setLevel = minCostServerLevel;
            newSolution.serverLevel[i] = setLevel;
        }
    }

    //计算该解
    calcSolution(newSolution, lastSolution);

    return newSolution;
}

/*************** 接受函数 ******************/

/* 局部变量 */
static solutionPack optSolution;

bool Accept(const solutionPack &newSolution, const solutionPack &originalSolu, double T_cur)
{
    //不接受无解
    if (newSolution.feasible == false)
        return false;

    if (newSolution.fixTotalProfit > optSolution.fixTotalProfit)
    {
        optSolution = newSolution;
    }
    if (newSolution.fixTotalProfit > originalSolu.fixTotalProfit)
    {
        return true;
    }
    if (newSolution.fixTotalProfit == originalSolu.fixTotalProfit)
    {
        if (newSolution.serverChangeCost < originalSolu.serverChangeCost)
            return true;
    }

    double Pt;

    Pt = exp(( - originalSolu.fixTotalProfit + newSolution.fixTotalProfit) / (T_cur));

    if (rand01() <= Pt)
    {
        // printf("Accept\n");
        return true;
    }
    else
    {
        return false;
    }
}

/********************* 输入一个费用超过余额的不合法的解, 削减消费者和服务器, 输出一个费用少于余额的解 *************************/
solutionPack reduceCost(solutionPack solution, const solutionPack &lastSolution){
    while (solution.serverChangeCost + solution.flowCost >= myMoney)
    {
        int minFlow = INT_MAX;
        int minFlowID = 0;
        for(auto i:solution.serverFlow){
            int id = i.first;
            int flow = i.second;
            if(flow < minFlow){
                if (_config.sale_server == false && lastSolution.solution.at(id) == true)
                    continue;
                minFlow = flow;
                minFlowID = id;
            }
        }
        solution.solution[minFlowID] = 0;
        solution.serverLevel.erase(minFlowID);
        solution.serverFlow.clear();
        solution.serverDeployTime.clear();
        calcSolution(solution, lastSolution, false, true, true);
        printf("[削减支出] 当前服务器变动费用:%d, 当前费用流费用:%d, 总费用:%d\n", solution.serverChangeCost, solution.flowCost, solution.flowCost + solution.serverChangeCost);
    }
    return solution;
}

/******************** SA主函数 ***********************/

solutionPack SA(solutionPack &basicSolution, const double T, long timeLimit, SA_config config, const solutionPack lastSolution){
    double SPEED ;
    double T_cur = T;
    //最终温度设为40度, 改善平均性能
    SPEED = pow(40.0/T, 1.0/timeLimit);
    long SA_beginTime = getMsClock();
    long printClock = 0;
    minCostFlowCount = 0;
    _config = config;

    //重新计算上一轮的利润
    fixRoundProfit(basicSolution);
    if(deployClock !=0) {
        printf("上一轮余额计算结果:%d, 实际%d\n", last_myMoney - lastSolution.serverChangeCost + basicSolution.roundProfit , myMoney);
        if(last_myMoney - lastSolution.serverChangeCost + basicSolution.roundProfit != myMoney){
            printf("[警告] 余额错误!\n");
        }
        printf("上一轮服务器变动成本:%d, 上一轮实际净利润:%d, 上一轮链路租金:%d\n", lastSolution.serverChangeCost, basicSolution.roundProfit, lastSolution.flowCost);
    }
    //由于上一次的解只是猜测的消费者节点的流量值, 所以不一定计算的是正确的, analysis已经修复了 basicSolution 中实际上没有赚取费用的消费者, 我们在这里重新计算roundProfit
    //这个basicSolution的消费者被减少了, 服务器没有改变, 
    calcSolution(basicSolution, lastSolution, false, false);
    if(basicSolution.feasible == false) {
        printf("[警告] 初始解不合法\n");
        basicSolution = reduceCost(basicSolution, lastSolution);
    }
    printf("基础解信息\n");
    printf("服务器数量:%d, 消费者数量%d, 服务器变动成本%d, 轮净利润%d, 费用流:%d, 总利润:%d\n",
           (int)basicSolution.serverLevel.size(), (int)basicSolution.consumerNeedMap.size(), basicSolution.serverChangeCost,
           basicSolution.roundProfit, basicSolution.flowCost, basicSolution.totalProfit);
    solutionPack searchSolution = basicSolution;
    optSolution = basicSolution;
    while ((getMsClock() - SA_beginTime) < timeLimit )
    {
        solutionPack newSolution = SA_getANewSolution(searchSolution, lastSolution, basicSolution);
        if (Accept(newSolution, searchSolution, T_cur))
        {
            searchSolution = newSolution;
        }
        long runTime = (getMsClock() - SA_beginTime) ;
        T_cur = T * pow((double)SPEED, (double)runTime);
        if (printClock < runTime){
            printClock += 1000 ;
            printf("混合目标:%d \t最优总利润(支出)(奖励)：" /*RED*/ "%d(%d)(%d) \t" /*NONE*/
                   " 当前：%d 本次：%d \t 温度：%lf \t 时间：%ld ms\n", optSolution.fixTotalProfit,
                   optSolution.totalProfit, optSolution.serverChangeCost + optSolution.flowCost, 
                   optSolution.fixProfit,
                   searchSolution.totalProfit, newSolution.totalProfit, T_cur, runTime);
        }
    }

    if(_config.check_myMoney == false){
        optSolution = reduceCost(optSolution, lastSolution);
    }

    printf("最优解信息:\n");
    printf("租金%d 服务器数%d 消费者数%d\n", optSolution.flowCost, (int)optSolution.serverLevel.size(), (int)optSolution.consumerNeedMap.size());
    printf("轮净利润(无部署费): %d , 部署费用%d\n", optSolution.roundProfit, optSolution.serverChangeCost);
    printf("猜测下一轮资金:%d\n", myMoney - optSolution.serverChangeCost + optSolution.roundProfit);
    printf("猜测最终余额:%d\n", myMoney + optSolution.totalProfit);
    printf("费用流运行:%d次\n", minCostFlowCount);
    //重新计算最优解 , 保证网络流与最优解对应
    calcSolution(optSolution, lastSolution, true);
    for(auto i:optSolution.serverFlow){
        if(i.second == 0) {
            printf("[!] 输出方案中含有流量为0的服务器%d\n", i.first);
        }
    }


    // for(auto i:optSolution.consumerNeedMap){         
    //     printf("%d : %d\n", i.first, i.second);
    // }

    return optSolution;
}


/****************** 获取一个直连解 ********************/
void getDirectSolution(solutionPack &solution)
{
    solutionPack newsolution;
    int consumer = consumerInfo.at(0).at(1);
    // int need = consumerInfo.at(0).at(2);
    newsolution.serverLevel[consumer] = maxServerLevel;
    newsolution.consumerNeedMap[consumer] = maxServerCapacity;
    solution = newsolution;
    runMinCostFlow(newsolution.serverLevel, newsolution.consumerNeedMap);
}


/******************* 打印每一轮部署情况 *********************/
void printSAresult(const solutionPack &deployPlan, const solutionPack &lastSolution)
{
    printf("本轮部署情况:\n");
    printf("服务器: \n");
    int add = 0, del = 0, mod = 0;
    for (int i = 0; i < vertexNum; i++)
    {
        if (lastSolution.solution.at(i) != deployPlan.solution.at(i))
        {
            if (deployPlan.solution.at(i))
            {
                printf("\t[+] %4d:%4d \n", i, deployPlan.serverLevel.at(i));
                add++;
            }
            else
            {
                printf("\t[-] %4d \n", i);
                del++;
            }
        }
        else if (lastSolution.solution.at(i) == 1 && deployPlan.solution.at(i) == 1)
        {
            if (lastSolution.serverLevel.at(i) != deployPlan.serverLevel.at(i)){
                printf("\t[*] %4d:%4d->%4d\n", i, lastSolution.serverLevel.at(i), deployPlan.serverLevel.at(i));
                mod++;
            }
        }
    }
    printf("\t 增 %d 删 %d 改 %d\n", add, del, mod);
    add = 0, del = 0, mod = 0;
    printf("消费者: \n");
    for (auto i : consumerAnalysisMap)
    {
        int consumerid = i.first;
        if (lastSolution.consumerNeedMap.count(consumerid) != deployPlan.consumerNeedMap.count(consumerid))
        {
            if (deployPlan.consumerNeedMap.count(consumerid))
            {
                printf("\t[+] %4d:%4d\n", consumerid, deployPlan.consumerNeedMap.at(consumerid));
                add++;
            }
            else
            {
                printf("\t[-] %4d\n", consumerid);
                del++;
            }
        }
        else if (lastSolution.consumerNeedMap.count(consumerid) == 1 && deployPlan.consumerNeedMap.count(consumerid) == 1)
        {
            if (lastSolution.consumerNeedMap.at(consumerid) != deployPlan.consumerNeedMap.at(consumerid))
            {
                printf("\t[*] %4d:%4d->%4d\n", consumerid, lastSolution.consumerNeedMap.at(consumerid), deployPlan.consumerNeedMap.at(consumerid));
                mod++;
            }
        }
    }
    printf("\t 增 %d 删 %d 改 %d\n", add, del, mod);
}
