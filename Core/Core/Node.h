#pragma once
#include<string>
#include<vector>
#include<memory>
#include <unordered_map>
struct TYPE
{
	void* pData; //универсал указатель
	size_t sizeData;
	bool (*compare) (TYPE); // шаблонная функция сравнения определенных типов данных (для юзера)
	bool operator == (TYPE compared) { return compare(compared); } //перегрузка операторов сравнения 
	bool operator != (TYPE compared) { return !(compare(compared)); } //перегрузка операторов сравнения 
};
struct Pair
{
	std::shared_ptr<Node> nearNode;
	double possOfSwitch; // вероятность перехода 
};

struct Node
{
	TYPE value;
	double possibility; 
	std::unordered_map<TYPE, std::shared_ptr<Pair>> nearestNodes;

};