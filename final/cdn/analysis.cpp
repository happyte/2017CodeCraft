#include "general_head.h"
#include "data.h"
#include "SA.h"
#include "analysis.h"
#include <cmath>
#include <cassert>


/************* 分析消费者节点需求 *************/

/************ 函数功能 *************/
/*
1. 根据已知信息评估节点当前的流量大小, 这个流量应该比竞争对手所供流量大, 并且大于所需最低流量
2. 修复上一次的部署方案, 由于上一次的部署方案可能有些节点并没有赚取流量, 这里去掉那些没有赚取流量的节点
    这样SA算法会重新运行基础解, 评估基础解的轮净利润(不含部署费用), 否则这个值可能是错误的
    修复的内容是 basicSolution.consumerNeedMap 上一次这个节点并没有贡献fee
*/


/**** 全局变量 *****/
map<int, int> consumerAnalysisMap;
multimap<int, int> consumerAnalysisMultimap;
set<int> competitorConsumerSet;
set<int> myConsumerSet;
set<int> newCompetitorConsumerSet;
set<int> newMyConsumerSet;
set<int> competitorConsumerHistorySet;
int balanceNum = 0;


/** 局部变量 ***/
set<int> last_competitorConsumerSet;
set<int> last_myConsumerSet;

/******* 参数 *********/
const int minAddFlow = 10;
const int plusFlow = 20;

map<int, int> allConsumerIHaveTry;

//我没抢到的分为这三种情况：这个没抢到只是跟上一轮比的
//我抢了，失败了   我没抢，他抢了   我没抢，他也没抢
//我抢到了的，是不会再动的
map<int, int> iTryButFailedMap;
map<int, int> iNotTryAndTheyGetMap;
map<int, int> weAllNotTryMap;

static int getConsumerBelong(int consuemrId)
{
    for (auto ite : consumerInfo){
        int id = ite.at(1);
        int consumerBelong = ite.at(3);
        if (id == consuemrId){
            return consumerBelong;
        }
    }
    printf("Error : can't get consumerId:%d\n",consuemrId);
    return -1;
}

void runAnalysisMap2(const  solutionPack &basicSolution, const solutionPack &deployPlan){
    int totalEarnConsumerNum = 0;
    int theirOwnConsumerNum = 0;
    int iTryButFailed = 0;
    int iTryAndSuccess = 0;
    int iNotTryAndTheyGet = 0;
    int weAllNotTry = 0;

    iTryButFailedMap.clear();
    iNotTryAndTheyGetMap.clear();
    weAllNotTryMap.clear();

    //遍历所有的800个消费节点
    for ( auto ite : consumerInfo){
        int consumerId = ite.at(1);
        int consumerOriginalMinNeed = ite.at(2);
        int consumerBelong = ite.at(3);
//danji

        //按照上一轮的 *实际结果* 来进行分析
        if (deployPlan.consumerNeedMap.count(consumerId) != 0  && consumerBelong != myID)
        {
            //1. 我部署了流量去抢，但是抢失败了（被对手抢走了）
            theirOwnConsumerNum++;
            iTryButFailed++;

            //更新消费者需求
            if(allConsumerIHaveTry.count(consumerId) == 0){

                allConsumerIHaveTry[consumerId] = deployPlan.consumerNeedMap.at(consumerId) + plusFlow;

            }else{

                allConsumerIHaveTry[consumerId] = allConsumerIHaveTry[consumerId] + plusFlow;
            }

            // basicSolution.consumerNeedMap.erase(consumerId);

            iTryButFailedMap[consumerId] = allConsumerIHaveTry[consumerId];

        }
        else if (deployPlan.consumerNeedMap.count(consumerId) != 0 && consumerBelong == myID)
        {
            //2. 我部署了流量去抢，确实抢到了(对手可能跟本没抢，也可能没抢过我们)
            totalEarnConsumerNum++;
            iTryAndSuccess++;
            allConsumerIHaveTry[consumerId] = deployPlan.consumerNeedMap.at(consumerId);//增加了这一句，看还会不会出现bug

        }
        else if (deployPlan.consumerNeedMap.count(consumerId) == 0 && consumerBelong != 0)
        {
            //3. 我没抢，对手抢了的
            theirOwnConsumerNum++;
            iNotTryAndTheyGet++;

            if(allConsumerIHaveTry.count(consumerId) != 0){//要看原来抢没抢过. 上一次没抢，不代表之前没抢过

                allConsumerIHaveTry[consumerId] += plusFlow;
                iNotTryAndTheyGetMap[consumerId] = allConsumerIHaveTry[consumerId];

            }else{

                iNotTryAndTheyGetMap[consumerId] = consumerOriginalMinNeed + rand() % minAddFlow;
            }
        }
        else{

            //4. 我没抢，对手也没抢
            weAllNotTry++;
            if (allConsumerIHaveTry.count(consumerId) != 0){

                allConsumerIHaveTry[consumerId] += plusFlow;
                weAllNotTryMap[consumerId] = allConsumerIHaveTry[consumerId];

            }else{

                weAllNotTryMap[consumerId] = consumerOriginalMinNeed + rand() % minAddFlow;
            }
        }
    }

    // for(auto &i:basicSolution.consumerNeedMap){
    //     i.second = allConsumerIHaveTry.at(i.first);
    // }
    cout << "test bottom" << endl;

    printf("上一轮实际占领的消费者:%d , %d \t 对手占领的消费者:%d \t 我抢了，失败了:%d \t 我抢了，成功了:%d \t 我没抢，他抢了:%d \t 我没抢，他也没抢:%d \n",
           (int)basicSolution.consumerNeedMap.size(), totalEarnConsumerNum, theirOwnConsumerNum, iTryButFailed, iTryAndSuccess, iNotTryAndTheyGet, weAllNotTry);
}

int totalEarnConsumerNum = 0;

void runAnalysisMap(solutionPack &basicSolution, const solutionPack &deployPlan){
    int theirOwnerConsumerNum = 0;
    competitorConsumerSet.clear();
    myConsumerSet.clear();
    newCompetitorConsumerSet.clear();
    newMyConsumerSet.clear();
    totalEarnConsumerNum = 0;
    static bool first = true;

    for(auto i: consumerInfo){
        //观察每一个消费者
        //如果这个消费者属于我们, 那么流量就是最低消费流量
        //如果这个消费者经常变更主人,那么流量应该是比较大的
        int consumerId = i.at(1);
        int consumerNeed = i.at(2);
        int consumerBelong = i.at(3);

        //无人归属, 则设置为流量的1倍
        // totalEarnConsumerNum = deployPlan.consumerNeedMap.size();
        // if(consumerBelong == 0){
        //     consumerAnalysisMap[consumerId] = consumerNeed + minAddFlow;
        // }
        // else

        //统计我们的消费者节点
        if(consumerBelong == myID){
            totalEarnConsumerNum++;
            myConsumerSet.insert(consumerId);
            if(last_myConsumerSet.count(consumerId) == 0){
                newMyConsumerSet.insert(consumerId);
            }
        }
        //统计对方的消费者节点
        if(consumerBelong != myID && consumerBelong != 0){
            theirOwnerConsumerNum++;
            competitorConsumerSet.insert(consumerId);
            competitorConsumerHistorySet.insert(consumerId);
            if(last_competitorConsumerSet.count(consumerId) == 0){
                newCompetitorConsumerSet.insert(consumerId);
            }            
        }

        //更改需要变更的猜测流量 

        //第一种情况 我们从未分析过流量, 默认设置为 consumerNeed 加随机数
        if(consumerAnalysisMap.count(consumerId) == 0)
        {
            if(consumerNeed > consumerMaxNeed.at(consumerId)){
                if(first) printf("永远无法满足的节点 %d \n", consumerId);
            }
            else{
                consumerAnalysisMap[consumerId] = consumerNeed + minAddFlow / consumerNeed + 1  + rand() % 4;
                
            }
        }
        //第二种情况 我们上次设置了流量, 但仍不属于我们
        else if(basicSolution.consumerNeedMap.count(consumerId) == 1 && consumerBelong != myID ) {

            //但是 如果上次这个解是最大流 那我们还是要的 
            if(basicSolution.consumerNeedMap.at(consumerId) == consumerMaxNeed.at(consumerId)){
                
            }
            //否则 继续在上面加流量 然后删掉基础解中这个没有盈利的解
            else{
                // if(consumerNeed > 50) 
                // {   //大流量 每次加 一半
                //     consumerAnalysisMap[consumerId] = consumerAnalysisMap[consumerId] + (consumerMaxNeed.at(consumerId) - consumerNeed)  / 2 + 1;
                // }
                // else{
                    //小流量 直接给最大值
                    consumerAnalysisMap[consumerId] = consumerMaxNeed.at(consumerId);
                // }
                basicSolution.consumerNeedMap.erase(consumerId);
            }
        }
        //第三种情况 对方新增的消费者节点
        else if(newCompetitorConsumerSet.count(consumerId) == 1 && consumerBelong != myID){
            // if (consumerNeed > 50)
            // { //大流量 每次加 一半
            //     consumerAnalysisMap[consumerId] = consumerAnalysisMap[consumerId] + (consumerMaxNeed.at(consumerId) - consumerNeed)  / 2 + 1;
            // }
            // else
            // {
                //小流量 直接给最大值
                consumerAnalysisMap[consumerId] = consumerMaxNeed.at(consumerId);
            // }
        }
        //修改完 一定要控制需求
        consumerAnalysisMap[consumerId] = min(consumerAnalysisMap[consumerId], consumerMaxNeed.at(consumerId));
    }

    // 基础解中仍存在的节点流量必然保持不变, 也就是抢到的消费者流量不变
    for(auto i:basicSolution.consumerNeedMap){
        // i.second = consumerAnalysisMap.at(i.first);
        assert(consumerAnalysisMap.at(i.first) == i.second);
    }

    consumerAnalysisMultimap.clear();
    for(auto i: consumerAnalysisMap){
        consumerAnalysisMultimap.insert(pair<int, int>(i.second, i.first));
    }

    printf("对方节点:\n");
    for(auto i : competitorConsumerSet){
        printf(" %d(%d) ", i , consumerAnalysisMap.at(i));
    }
    printf("\n");

    printf("对方新增节点:\n");
    for(auto i : newCompetitorConsumerSet){
        printf(" %d(%d) ", i, consumerAnalysisMap.at(i));
    }
    printf("\n");

    printf("我方节点:\n");
    for(auto i : myConsumerSet){
        printf(" %d(%d) ", i, consumerAnalysisMap.at(i));
    }
    printf("\n");

    printf("我方新增节点:\n");
    for(auto i : newMyConsumerSet){
        printf(" %d(%d) ", i, consumerAnalysisMap.at(i));
    }
    printf("\n");

    printf("上一轮实际占领的消费者:%d , \t 对手占领的消费者:%d  \t 双方都占了且为容量上限 %d\n", totalEarnConsumerNum, theirOwnerConsumerNum, (int)basicSolution.consumerNeedMap.size() - totalEarnConsumerNum);
    balanceNum = (int)basicSolution.consumerNeedMap.size() - totalEarnConsumerNum;
    balanceNum*=2;
    printf("猜测双方都有的最大上限节点:%d\n", balanceNum);
    last_competitorConsumerSet = competitorConsumerSet;
    last_myConsumerSet = myConsumerSet;
    first = false;
}