#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "lib_io.h"
#ifdef __cplusplus
extern "C" {
#endif
void deploy_server(char* inLines[MAX_IN_NUM], int inLineNum, const char * const filename);

#ifdef __cplusplus
}
#endif

#endif
