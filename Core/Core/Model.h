#pragma once
#include"Node.h"
#define DATA_SET void* //заглушка
class Model
{
public:
	Model() = default;
	Model(DATA_SET dataSet, TYPE*(*operationsWDataSet)(DATA_SET dataSet));
	void clear();
	void devModel(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet));
	size_t Size();
	TYPE* retArrValue();
};

