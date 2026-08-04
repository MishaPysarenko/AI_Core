#pragma once
#include"Node.h"
#include <unordered_map>
#define DATA_SET void* //„ÎÛ¯ËÎÍ‡
class Model
{
	std::unordered_map<TYPE, std::shared_ptr<Node>> valueModel;
public:
	Model() = default;
	Model(DATA_SET dataSet, TYPE*(*operationsWDataSet)(DATA_SET dataSet));
	void clear();
	void createModel(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet));
	size_t Size();
	TYPE* retArrValue();
	double** retMatrix—onnexion(); 

};

