#include "Model.h"

Model::Model(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet))
{
}

void Model::createModel(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet))
{

}

size_t Model::Size()
{
	return valueModel.size();
}

void Model::clear() 
{
	valueModel.clear();
}

TYPE* Model::retArrValue()
{
	TYPE* value = new TYPE[valueModel.size()];
	size_t iterator = 0;
	for (auto pair: valueModel)
	{
		value[iterator] = pair.first;
		iterator++;
	}
	return value;
}


//DZ недалеко от функции свыше 
double** Model::retMatrixСonnexion()
{
	return nullptr;
}


