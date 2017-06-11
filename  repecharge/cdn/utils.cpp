#ifndef _UTILS_H
#define _UTILS_H

#include "utils.h"
#include "deploy.h"
#include <sstream>
using namespace std;

void split(const string &src, const string &delim, vector<int> &destination)
{
    string str = src;
    string::size_type start = 0, index;
    string substr;

    index = str.find_first_of(delim, start); //在str中查找(起始：start) delim的任意字符的第一次出现的位置
    while (index != string::npos)
    {
	substr = str.substr(start, index - start);
	stringstream ss(substr);
	int ret;
	ss >> ret;
	destination.push_back(ret);
	start = str.find_first_not_of(delim, index); //在str中查找(起始：index) 第一个不属于delim的字符出现的位置
	if (start == string::npos)
	    return;

	index = str.find_first_of(delim, start);
    }
    if (index == string::npos)
    {
	substr = str.substr(start, str.size() - start);
	stringstream ss(substr);
	int ret;
	ss >> ret;
	destination.push_back(ret);
    }
}

#endif