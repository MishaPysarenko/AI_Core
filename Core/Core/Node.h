#pragma once
#include<string>
#include<vector>
#include<memory>
struct TYPE
{
	void* pData;
	size_t sizeData;
};

struct Node
{
	TYPE value;
	double possibility; // вероятность перехода 
	std::vector<std::shared_ptr<Node>> nearestNodes;

};