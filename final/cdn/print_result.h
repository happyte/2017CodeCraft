#ifndef __PRINT_RESULT_H
#define __PRINT_RESULT_H
#include <map>
/* 打印图中的最小费用流到字符串resultInOneLine中 */
/* 输入： 超级源点 S_ID 超级汇点 T_ID 消费者反查数据consumerID*/
/* 输出： 打印结果 resultInOneLine */
/* 引用函数 */
// extern int getArcFlow(int sourceNodeId, int targetNodeId);
// extern void getOutgoningArcTargetNodeId(int sourceNodeId, std::vector<int>& result);

void printMinFlowResult(std::string &resultInOneLine, int S_ID, int T_ID, const map<int, int> &consumerID,
                        const map<int, int> &serverLevel, const set<int> &zeroServer);

#endif