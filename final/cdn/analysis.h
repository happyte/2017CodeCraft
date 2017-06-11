#ifndef _ANALYSIS_H
#define _ANALYSIS_H

/************* 分析消费者节点需求 *************/

/**** 全局变量 *****/
extern map<int, int> consumerAnalysisMap;
extern multimap<int, int> consumerAnalysisMultimap;
extern set<int> competitorConsumerSet;
extern set<int> competitorConsumerHistorySet;
extern int totalEarnConsumerNum;
extern int balanceNum;
void runAnalysisMap(solutionPack &lastSolution, const solutionPack &deployPlan);
void runAnalysisMap2(const solutionPack &basicSolution, const solutionPack &deployPlan);

#endif