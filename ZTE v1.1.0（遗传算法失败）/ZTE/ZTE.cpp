// ZTE.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include "pch.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include <time.h>

using namespace std;
//存放原始拓扑信息
vector<int> gridTopo[956];
//存放所有业务需求信息
vector<int> request[4001];

void clearVec()
{
	for (int i = 0; i < 956; i++) gridTopo[i].clear();
	for (int i = 0; i < 4001; i++) request[i].clear();
}
void readTxt()
{
	char readLine[1000];
	const char *delim = " ";
	char *p;
	ifstream infile;
	string str;

	infile.open("map/gridtopoAndRequest.txt");
	for (int i = 0; i < 956; i++)
	{
		//cin.getline(readLine, 1000);
		getline(infile, str);
		strcpy(readLine, str.c_str());
		p = strtok(readLine, delim);
		while (p)
		{
			gridTopo[i].push_back(atoi(p));
			p = strtok(NULL, delim);
		}
	}
	for (int i = 0; i < 4001; i++)
	{
		//cin.getline(readLine, 1000);
		getline(infile, str);
		strcpy(readLine, str.c_str());
		p = strtok(readLine, delim);
		while (p)
		{
			request[i].push_back(atoi(p));
			p = strtok(NULL, delim);
		}
	}
	infile.close();
}

class Request
{
public:
	int requestID;			//业务ID
	int reqeustBD;			//业务请求带宽
	int beginID;			//业务起点
	int endID;				//业务终点
	int cost;				//业务传输所需的花费
	vector<int> road_cross;	//记录业务的路径

	Request();				//默认构造函数
	//~Request();

private:

};

Request::Request()
{
	this->requestID = 0;
	this->reqeustBD = 0;
	this->beginID = 0;
	this->endID = 0;
	this->cost = 0;
}

class Cross
{
public:
	int crossID;			//节点ID
	vector<int> idnex;		//存放当前节点可走的链路在gridTopo数组的索引位置

	Cross();
	//~Cross();

private:

};

Cross::Cross()
{
	this->crossID = -1;
}

class NodeData
{
public:
	int now_cross;		//当前节点
	int pre_cross;		//当前节点的前导节点
	int g_n;			//起点到候选节点实际花费的代价
	int h_n;			//候选节点到目标点D的估计代价，一般为马氏距离或欧氏距离
	int f_n;			//总的值，f_n=g_n+h_n

	NodeData();
	//~NodeData();

private:

};

NodeData::NodeData()
{
	this->now_cross = -1;
	this->pre_cross = -1;
	this->g_n = 0;
	this->h_n = 0;
	this->f_n = 0;
}

//节点状态
enum State { nostate, inopen, inclose };
//记录节点状态及在open或close表中的位置，减少查询次数
class CrossState
{
public:
	State state;		//节点状态信息
	int index;			//在open或close表中的位置

	CrossState();
	//~CrossState();

private:

};

CrossState::CrossState()
{
	this->state = nostate;
	this->index = -1;
}

//全局变量声明
Request *request_class;			//存放所有业务需求数据的类数组
Cross *cross_class;				//存放节点信息的类数组
int **linkroad_use;				//记录链路的使用量；正反两个方向

//将所有业务数据存放到类数组中
void GetRequestData()
{
	int request_count = 0;		//总的业务数
	int backup_count = 0;		//每种业务的备用路径数
	int request_index = 1;		//业务在vector的索引，从1开始。

	request_count = request[0][0];
	backup_count = request[0][1];
	request_class = new Request[request_count];
	for(int i = 0; i < 1000; i++)
	{
		request_class[i].requestID = request[request_index][0];
		request_class[i].reqeustBD = request[request_index][1];
		request_index++;		//路径存储在下几个索引位置
		request_class[i].beginID   = request[request_index].front();
		request_class[i].endID     = request[request_index].back();
		request_index += backup_count;	//切换到下一业务
//		cout << request_class[i].requestID << " " << request_class[i].reqeustBD << " " <<
//			request_class[i].beginID << " " << request_class[i].endID << endl;
	}
}

//寻找节点值为crossID的节点是否在节点类数组中，如果在，返回索引位置；若不在，返回-1
int Find(Cross *cross, int crossID)
{
	for (int i = 0; i < gridTopo[0][0]; i++)
		if (cross[i].crossID == crossID)
			return i;
	return -1;					//没找到。返回-1
}

//寻找某条链路是否已被收录在某个节点中了
int Find(vector<int> index, int road_index)
{
	int index_temp = 0;
	vector<int>::iterator it = index.begin();

	while(it != index.end())
	{
		if (*it == road_index)
			return index_temp;
		index_temp++;
		it++;
	}
	return -1;					//没找到，返回-1
}

//寻找某个节点是否已经在另一个方向搜索的open表中了
int Find(vector<NodeData> open, int crossID)
{
	//从后向开始搜索。加快搜索速度
	for (int i = open.size() - 1; i >= 0; i--)
		if (open[i].now_cross == crossID)
			return i;
	return -1;					//没找到，返回-1
}

//寻找某条业务在request_class数组中的索引位置
int Find(Request *request_class, int requestID)
{
	for (int i = 0; i < request[0][0]; i++)
		if (request_class[i].requestID == requestID)
			return i;
	return -1;					//没找到，返回-1
}

//根据两个节点，寻找链路索引
int Find(int pre_cross, int next_cross)
{
	int index_cross = Find(cross_class, pre_cross);
	for(vector<int>::iterator it = cross_class[index_cross].idnex.begin(); it != cross_class[index_cross].idnex.end(); it++)
		if (gridTopo[*it][0] == next_cross || gridTopo[*it][1] == next_cross)
			return *it;
	return -1;
}

//获取所有节点信息并存放在类数组中
void GetCrossData(void)
{
	int cross_count = 0;		//节点数
	int linkroad_count = 0;		//链路数

	cross_count = gridTopo[0][0];
	linkroad_count = gridTopo[0][1];
	cross_class = new Cross[cross_count];
	int index_cross = 0;
	for(int i = 1; i < linkroad_count + 1; i++)			//第一行为数据数量信息，第二行开始才是正式的链路信息;结尾也应加一行
	{
		int temp = Find(cross_class, gridTopo[i][0]);	//链路起点
		if(temp == -1)			//当前链路的起点没有被收录
		{
			cross_class[index_cross].crossID = gridTopo[i][0];
			//将当前节点可行路径加入index中，因为当前节点未被收录，所以这个链路一定没被收录
			cross_class[index_cross].idnex.push_back(i);
			index_cross++;
		}
		else if(Find(cross_class[temp].idnex, i) == -1)	//当前链路未被收录到目标节点中
		{
			cross_class[temp].idnex.push_back(i);
		}
		temp = Find(cross_class, gridTopo[i][1]);		//链路终点
		if (temp == -1)			//当前链路的终点没有被收录
		{
			cross_class[index_cross].crossID = gridTopo[i][1];
			//将当前节点可行路径加入index中，因为当前节点未被收录，所以这个链路一定没被收录
			cross_class[index_cross].idnex.push_back(i);
			index_cross++;
		}
		else if (Find(cross_class[temp].idnex, i) == -1)	//当前链路未被收录到目标节点中
		{
			cross_class[temp].idnex.push_back(i);
		}
	}
}

//寻找open表中最小的一个值，并返回索引位置
int FindMin(vector<NodeData> open)
{
	int index = 0;
	int min = open[0].f_n;

	for(int i = 1; i < open.size(); i++)
	{
		if(open[i].f_n < min)
		{
			min = open[i].f_n;
			index = i;
		}
	}
	return index;
}

//双向A*算法的实现
int BiAStar(Request request, vector<NodeData> open[2], vector<NodeData> close[2], CrossState **crossstate, bool isback)
{
	int x[2], y[2];
	int g_n, h_n;
	int cross_now = close[isback].back().now_cross;			//当前最后一个已确定的节点
	int index_cross = Find(cross_class, cross_now);			//获取节点在数组中的索引

	if (request.requestID == 585 && cross_now == 26)
		int test = 0;

	int cross_temp = -1;		//存放目标节点。起点或终点
	if (isback == false)		//前向
		cross_temp = request.endID;
	else						//反向
		cross_temp = request.beginID;
	x[0] = cross_temp % 20;
	y[0] = cross_temp / 20;
	//for(vector<int>::iterator it = cross_class[index_cross].idnex.begin(); it != cross_class[index_cross].idnex.end(); it++)
	for(int i = 0; i < cross_class[index_cross].idnex.size(); i++)
	{
		int cross_next = 0;			//下一个节点
		if (gridTopo[cross_class[index_cross].idnex[i]][0] == cross_now)
			cross_next = gridTopo[cross_class[index_cross].idnex[i]][1];
		else
			cross_next = gridTopo[cross_class[index_cross].idnex[i]][0];
		//判断下一节点是否已被收录到close表中，若在，则跳过当前链路
		if(crossstate[isback][cross_next].state == inclose)
			continue;
		//判断链路上的流量是否超过链路带宽的80%;超过，则跳过当前链路
		int temp_linkroad = 0;
		if(isback == false)			//前向搜索
		{
			if (gridTopo[cross_class[index_cross].idnex[i]][0] == cross_now)
				temp_linkroad = linkroad_use[0][cross_class[index_cross].idnex[i] - 1] + request.reqeustBD;
			else
				temp_linkroad = linkroad_use[1][cross_class[index_cross].idnex[i] - 1] + request.reqeustBD;
		}
		else						//后向搜索
		{
			if (gridTopo[cross_class[index_cross].idnex[i]][0] == cross_next) //对于后向搜索，cross_next才是头点
				temp_linkroad = linkroad_use[0][cross_class[index_cross].idnex[i] - 1] + request.reqeustBD;
			else
				temp_linkroad = linkroad_use[1][cross_class[index_cross].idnex[i] - 1] + request.reqeustBD;
		}
		if (temp_linkroad / (float)gridTopo[cross_class[index_cross].idnex[i]][2] >= 0.8)
			continue;
		//计算g_n、h_n
		x[1] = cross_next % 20;
		y[1] = cross_next / 20;
		h_n = abs(x[0] - x[1]) + abs(y[0] - y[1]);
		g_n = close[isback].back().g_n + request.reqeustBD*gridTopo[cross_class[index_cross].idnex[i]][3];
		NodeData nodedata;
		nodedata.g_n = g_n;
		nodedata.h_n = h_n;
		nodedata.f_n = g_n + h_n;
		nodedata.pre_cross = cross_now;
		nodedata.now_cross = cross_next;
		//计算相关后继节点的信息并加入open表中，如果已在open表中，更新信息（如果需要）
		if(crossstate[isback][cross_next].state == nostate)		//下一节点未在当前搜索方向的open表中
		{
			open[isback].push_back(nodedata);
			crossstate[isback][cross_next].state = inopen;
		}
		else						//下一节点已在open表中，计算代价f_n，与之前的比较，如果小于，便更新
		{
			int index_open = Find(open[isback], cross_next);		//寻找下一节点是否在自己搜索方向的open表中
			if (g_n + h_n < open[isback][index_open].f_n)
			{
				open[isback][index_open].g_n = g_n;
				open[isback][index_open].h_n = h_n;
				open[isback][index_open].f_n = g_n + h_n;
				open[isback][index_open].pre_cross = cross_now;
				open[isback][index_open].now_cross = cross_next;
			}
		}
	}
	//将open表中代价最小的一个加入close表中并在open表中删除
	int index_open = FindMin(open[isback]);
	close[isback].push_back(open[isback][index_open]);
	crossstate[isback][open[isback][index_open].now_cross].state = inclose;
	if (index_open + 1 == open[isback].size())		//所处位置是最后一个
		open[isback].pop_back();
	else
		open[isback].erase(open[isback].begin() + index_open, open[isback].begin() + index_open + 1);
	//判断路径寻找是否完成
	if(crossstate[!isback][close[isback].back().now_cross].state == inclose)
		return close[isback].back().now_cross;
	//切换到另外一个方向进行搜索
	int temp = BiAStar(request, open, close, crossstate, !isback);
	if (temp != -1)
		return temp;
	return -1;
}

//根据open、close表和会合节点，提取路径
void GetRequestRoad(int index_request, vector<NodeData> open[2], vector<NodeData> close[2], int middle_cross)
{
	int cost = 0;
	int cross_now = 0;

	//获取路径;求取总的花费，
	request_class[index_request].road_cross.push_back(middle_cross);
	for(int i = 0; i < 2; i++)
	{
		cross_now = middle_cross;
		int index = Find(close[i], middle_cross);
		cost += close[i][index].g_n;
		int pre_cross = close[i][index].pre_cross;
		while (pre_cross != -1)
		{
			if (i == 0)
				request_class[index_request].road_cross.insert(request_class[index_request].road_cross.begin(), pre_cross);
			else
				request_class[index_request].road_cross.push_back(pre_cross);
			//将业务要走的链路上的流量加上
			int index_road = Find(cross_now, pre_cross);
					//这个index_road表示的是在gridTopo中的索引位置，所以后面使用需要减1，第一行为节点和链路数量说明
			if(i == 0)		//由汇合点向前搜索，pre_cross才是头点
			{
				if(gridTopo[index_road][0] == pre_cross)	//正向
					linkroad_use[0][index_road - 1] += request_class[index_request].reqeustBD;
				else										//反向
					linkroad_use[1][index_road - 1] += request_class[index_request].reqeustBD;
			}
			else			//由会合点向后搜索，cross_now是头点
			{
				if (gridTopo[index_road][0] == cross_now)	//正向
					linkroad_use[0][index_road - 1] += request_class[index_request].reqeustBD;
				else										//反向
					linkroad_use[1][index_road - 1] += request_class[index_request].reqeustBD;
			}
			cross_now = pre_cross;
			//下一路径节点
			close[i].erase(close[i].begin() + index, close[i].begin() + index + 1);	//删除已提取节点
			index = Find(close[i], pre_cross);
			pre_cross = close[i][index].pre_cross;
		}
	}
	request_class[index_request].cost = cost;
}

//双向A*算法的前期准备
void FindRoad(Request request)
{
	vector<NodeData> open[2];		//候选节点
	vector<NodeData> close[2];		//已确定节点
	CrossState **crossstate;		//保存节点状态信息
	NodeData temp1, temp2;

	crossstate = new CrossState*[2];
	crossstate[0] = new CrossState[gridTopo[0][0]];
	crossstate[1] = new CrossState[gridTopo[0][0]];
	//前向搜索时，将业务起点加入close中
	temp1.now_cross = request.beginID;
	temp1.pre_cross = -1;
	close[0].push_back(temp1);
	crossstate[0][request.beginID].state = inclose;
	//后弦搜索时，将终点加入close中
	temp2.now_cross = request.endID;
	temp2.pre_cross = -1;
	close[1].push_back(temp2);
	crossstate[1][request.endID].state = inclose;
	//使用双向A*算法开始寻路并记录中间会合节点
	bool isback = false;
	int middle_cross = BiAStar(request, open, close, crossstate, isback);
	//while (middle_cross == -1)
	//{
	//	middle_cross = BiAStar(request, open, close, crossstate, isback);
	//	isback = !isback;
	//}
	int index = Find(request_class, request.requestID);
	GetRequestRoad(index, open, close, middle_cross);
}

//计算所有业务的总成本
int GetCost(void)
{
	int cost = 0;
	for (int i = 0; i < request[0][0]; i++)
		cost += request_class[i].cost;
	return cost;
}

//清除业务及道路各种信息
void ClearReqRoad(void)
{
	for (int i = 0; i < request[0][0]; i++)
	{
		request_class[i].cost = 0;
		request_class[i].road_cross.clear();
	}
	for (int i = 0; i < gridTopo[0][1]; i++)
	{
		linkroad_use[0][i] = 0;
		linkroad_use[1][i] = 0;
	}
}

//生成初始群体
void GenerateInitGroups(int **sequen, int M)
{
	bool* select;

	select = new bool[request[0][0]];
	srand((int)time(0));
	for(int i = 0; i < M; i++)
	{
		for (int i = 0; i < request[0][0]; i++)
			select[i] = false;
		for(int j = 0; j < request[0][0]; j++)
		{
			int temp = 0;
			do
			{
				temp = rand() % request[0][0];
			} while (select[temp]);
			select[temp] = true;
			sequen[i][j] = temp;
		}
	}
}

//计算适应度值
void CaculateFitness(int *fit_value, int **sequen, int M)
{
	int* value;

	value = new int[M];
	for(int i = 0; i < M; i++)
	{
		for (int j = 0; j < request[0][0]; j++)
			FindRoad(request_class[sequen[i][j]]);
		value[i] = GetCost();
		////测试
		cout << value[i] << " ";

		if (value[i] > 5000000)
			value[i] = 5000000;
		fit_value[i] = 5000000 - value[i];
		//fit_value[i] = 1 / (float)value[i] / 1000000.0;
		ClearReqRoad();
	}
}

//进行遗传算法的选择操作
void GASelect(int **sequen, int *fit_value, int M)
{
	int all_value = 0;
	float* P, * P_accm;					//个体的概率和累积概率

	P = new float[M];
	P_accm = new float[M];
	//计算总的适应度值
	for (int i = 0; i < M; i++)
		all_value += fit_value[i];
	//计算各个个体的概率
	for (int i = 0; i < M; i++)
		P[i] = fit_value[i] / (float)all_value;
	//计算累积概率
	P_accm[0] = P[0];
	for (int i = 1; i < M; i++)
		P_accm[i] = P_accm[i - 1] + P[i];
	//根据轮盘赌进行选择
	int** temp_sequen;
	temp_sequen = new int* [M];
	for (int i = 0; i < M; i++)
		temp_sequen[i] = new int[request[0][0]];
	srand((int)time(0));
	for (int i = 0; i < M; i++)
	{
		float temp = rand() % 10000 / 10000.0;		//生成[0,1]之间的一个小数
		int index = 0;
		if(temp > P_accm[0])
		{
			for (int i = 1; i < M; i++)
			{
				if (temp > P_accm[i - 1] && temp < P_accm[i])
				{
					index = i;
					break;
				}
			}
		}
		temp_sequen[i] = sequen[index];
	}
	//将选择的基因放回原序列
	for (int i = 0; i < M; i++)
		sequen[i] = temp_sequen[i];
}

//遗传算法的交叉操作（单点交叉）
void GACross(int **sequence, int M)
{
	int* gene_pair;					//进行交叉的基因对

	gene_pair = new int[M];
	//获得进行交叉的基因对
	srand((int)time(0));
	bool* select;
	select = new bool[M];
	for (int i = 0; i < M; i++)
		select[i] = false;
	for (int i = 0; i < M; i++)
	{
		int temp = 0;
		do
		{
			temp = rand() % M;
		} while (select[temp]);
		select[temp] = true;
		gene_pair[i] = temp;
	}
	//进行交叉操作
	for (int i = 0; i < M; i += 2)
	{
		int* temp_sequen = sequence[gene_pair[i + 1]];
		int index = rand() % request[0][0];				//获得交叉点的位置
		bool* select1;
		select1 = new bool[request[0][0]];
		////操作基因对的第二个基因
		for (int j = 0; j < request[0][0]; j++)
			select1[j] = false;
		//记录index前面的状态
		for (int j = 0; j < index; j++)
			select1[sequence[gene_pair[i + 1]][j]] = true;
		//交叉
		int temp_index = index;
		int k = 0;
		while (temp_index < request[0][0])
		{
			if(!select1[sequence[gene_pair[i]][k]])
			{
				select1[sequence[gene_pair[i]][k]] = true;
				sequence[gene_pair[i + 1]][temp_index] = sequence[gene_pair[i]][k];
				temp_index++;
			}
			k++;
		}
		////操作基因对的第一个基因
		for (int j = 0; j < request[0][0]; j++)
			select1[j] = false;
		//记录index前面的状态
		for (int j = 0; j < index; j++)
			select1[sequence[gene_pair[i]][j]] = true;
		//交叉
		temp_index = index;
		k = 0;
		while (temp_index < request[0][0])
		{
			if (!select1[temp_sequen[k]])
			{
				select1[temp_sequen[k]] = true;
				sequence[gene_pair[i]][temp_index] = temp_sequen[k];
				temp_index++;
			}
			k++;
		}
	}
}

//遗传算法的变异
void GAMutate(int **sequence, int M, float P_mutate)
{
	srand((int)time(0));
	for(int i = 0; i < M; i++)
	{
		float P = rand() % 10000 / 10000.0;			//随机生成[0,1]间的小数
		if (P > P_mutate)							//概率大于变异率，不进行变异
			continue;
		int n = 40;
		while (n >= 0)
		{
			int begin = rand() % request[0][0];			//变异起始位置
			int end = rand() % request[0][0];			//变异终点位置
			if (begin == end)
				continue;
			int temp = sequence[i][begin];
			sequence[i][begin] = sequence[i][end];
			sequence[i][end] = temp;
			n--;
		}
	}
}

//遗传算法求解最优序列
void GeneticAlgorithm(int* sequence)
{
	int Gen = 20;					//迭代次数
	int M = 6;						//种群个体数
	float P_mutate = 0.5;			//变异率
	int** sequen;
	int* fit_value;					//记录各个个体的适应度值

	sequen = new int* [M];
	for (int i = 0; i < M; i++)
		sequen[i] = new int[request[0][0]];
	fit_value = new int[M];
	//产生初始群体
	GenerateInitGroups(sequen, M);
	//开始进行选择、交叉、进化
	int* optimal_sequen;
	int optimal_value = 0;
	optimal_sequen = new int[request[0][0]];
	while (Gen >= 0)
	{
		//计算各个体的适应度值
		CaculateFitness(fit_value, sequen, M);
		////测试
		//for (int i = 0; i < M; i++)
		//	cout << 6000000 - fit_value[i] << " ";
		//记录最优个体
		for(int i = 0; i < M; i++)
		{
			if(fit_value[i] > optimal_value)
			{
				optimal_value = fit_value[i];
				optimal_sequen = sequen[i];
			}
		}
		////测试
		cout << "<" << 5000000 - optimal_value << "> ";
		//将最优个体替换最劣个体
		int index = 0;
		int temp_value = 6000000;
		for (int i = 0; i < M; i++)
		{
			if(fit_value[i] < temp_value)
			{
				index = i;
				temp_value = fit_value[i];
			}
		}
		sequen[index] = optimal_sequen;
		fit_value[index] = optimal_value;
		//根据轮盘赌进行遗传算法的选择操作
		GASelect(sequen, fit_value, M);
		//遗传算法的交叉操作
		GACross(sequen, M);
		//遗传算法的变异操作
		GAMutate(sequen, M, P_mutate);
		Gen--;
	}
	////最后检查最后一次结果是否有更优的结果
	//计算各个体的适应度值
	CaculateFitness(fit_value, sequen, M);
	//记录最优个体
	for (int i = 0; i < M; i++)
	{
		if (fit_value[i] > optimal_value)
		{
			optimal_value = fit_value[i];
			optimal_sequen = sequen[i];
		}
	}
	//输出最优序列
	sequence = optimal_sequen;
}

//获取所有业务的路径和花费
void GetAllRequestRoad(void)
{
	int deal_count = 0;
	int requestBD = 0, last_requestBD = 100000;

	//链路使用情况数组初始化
	linkroad_use = new int*[2];
	linkroad_use[0] = new int[gridTopo[0][1]];
	linkroad_use[1] = new int[gridTopo[0][1]];
	for (int i = 0; i < gridTopo[0][1]; i++)
	{
		linkroad_use[0][i] = 0;
		linkroad_use[1][i] = 0;
	}
	//寻找路径
	int* sequence;					//发车序列
	sequence = new int[request[0][0]];
	GeneticAlgorithm(sequence);
	for (int i = 0; i < request[0][0]; i++)
		FindRoad(request_class[sequence[i]]);
}

//显示总的花费和所有节点的路径
void DisplayRoad(void)
{
	int cost = 0;
	for (int i = 0; i < request[0][0]; i++)
		cost += request_class[i].cost;
	cout << cost << endl;
	for (int i = 0; i < request[0][0]; i++)
	{
		cout << request_class[i].requestID << " " << request_class[i].reqeustBD << endl;
		for (vector<int>::iterator it = request_class[i].road_cross.begin(); it != request_class[i].road_cross.end(); it++)
			cout << *it << " ";
		cout << endl;
	}

	//ofstream outfile;
	//outfile.open("map/result.txt");
	//outfile << cost << endl;
	//for (int i = 0; i < request[0][0]; i++)
	//{
	//	outfile << request_class[i].requestID << " " << request_class[i].reqeustBD << endl;
	//	for (vector<int>::iterator it = request_class[i].road_cross.begin(); it != request_class[i].road_cross.end(); it++)
	//		outfile << *it << " ";
	//	outfile << endl;
	//}
}

int main()
{
	clearVec();
	//1.输入
	readTxt();
	//2.write your code
	GetRequestData();			//将所有业务数据存放到类数组中
	GetCrossData();				//获取所有节点信息并存放在类数组中
	GetAllRequestRoad();
	DisplayRoad();

	return 0;
}
