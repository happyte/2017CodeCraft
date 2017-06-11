#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "lib_io.h"
#include <vector>
#include <iostream>
#include <climits>

void deploy_server(char * graph[MAX_EDGE_NUM], int edge_num, char * filename);

int getArcFlow(int sourceNodeId, int targetNodeId);
void getOutgoningArcTargetNodeId(int sourceNodeId, std::vector<int>& result);

#define MAXV 20000
	

#endif
