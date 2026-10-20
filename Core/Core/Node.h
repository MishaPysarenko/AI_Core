#pragma once
#include<string>
#include<vector>
#include<memory>
#include <unordered_map>
#include <stdexcept>
#include <functional>
struct TYPE
{
    void* pData = nullptr;
    size_t sizeData;

    bool (*compare)(TYPE, TYPE);
    size_t(*hash)(TYPE);

    bool operator==(const TYPE& compared) const
    {
        if (sizeData != compared.sizeData)
            return false;

        if (sizeData == 0)
            return true;

        if (!pData || !compared.pData)
            return false;

        return std::memcmp(pData, compared.pData, sizeData) == 0;
    }

    bool operator!=(const TYPE& compared) const
    {
        return !(*this == compared);
    }
};

template<>
struct std::hash<TYPE>
{
    size_t operator()(const TYPE& value) const
    {
        return value.hash(value);
    }
};

struct Node;

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