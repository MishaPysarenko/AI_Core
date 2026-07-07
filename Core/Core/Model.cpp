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



double** Model::retMatrixСonnexion()
{
	//1 создание асоциативного массива с определенным порядком значений и их индексов 
	double** connexion = new double*[valueModel.size()];
	TYPE* tempARR = retArrValue();
	std::unordered_map<size_t ,TYPE> array;
	for (size_t i = 0; i < valueModel.size(); i++)
		array[i] = tempARR[i];
	delete tempARR;

	//2 обычный цикл фор который итеррируеться с помощью обычного итератора по асоциативному массиву 
	for (size_t i = 0; i < array.size(); i++)
	{
		//3 каждую итерацию внешнего цикла фор создаем в матрице связей новый массив связей 
		connexion[i] = new double[array.size()];
		//4 после создание нового массива ищем ноду которую соответствует итератору нашого асоциативного массива 
		Node* val = valueModel.find(array.find(i)->second)->second;
		//5 теперь в внутренем цикле:(фор ауто или обычный) ходим по всем ближайшим нодам у ноды которую мы взяли ранее
		for (size_t i = 0; i < array.size(); i++)
		{
			//6 если взятая ближайшая нода совпадает с индексом нашего асоциативного массива записываем по индексу вероятность перехода к этой ноде 
			if (val->nearestNodes[i] == array.find(i)->second)//доделать
			{
				connexion[i][] = it->possOfSwitch;
			}
			//в противном случае записываем 0 
			else
			{
				connexion[i][] = 0;
			}
		}
		
	}
	return connexion;
}





