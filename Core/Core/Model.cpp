#include "Model.h"

Model::Model(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet))
{
	createModel(dataSet, operationsWDataSet);
}

void Model::createModel(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet))
{
	TYPE* arrayValue = operationsWDataSet(dataSet);
	size_t n = arrayValue[0].sizeData; //резервирование самого первого элемента массива под сайз(а не как указатель)

	for (size_t i = 1; i < n; i++)
	{
		if (valueModel.find(arrayValue[i]) == valueModel.end())
		{
			std::shared_ptr<Node> temp = std::make_shared<Node>();
			temp->value = arrayValue[i];
			temp->possibility = 1;
			valueModel[arrayValue[i]] = temp;

 		}
		else
		{
			auto temp = valueModel.find(arrayValue[i]);
			temp->second->possibility++;
		}

		if (n > 2)
		{
			auto current = valueModel.find(arrayValue[i])->second; // первый   
			auto successive = valueModel.find(arrayValue[i + 1])->second; // последующие

			auto temp = current->nearestNodes.find(successive->value);

			if (temp == current->nearestNodes.end())
			{
				std::shared_ptr<Pair> tempPair = std::make_shared<Pair>();
				tempPair->nearNode = successive;
				tempPair->possOfSwitch = 1;
				current->nearestNodes[successive->value] = tempPair;
			}
			else
			{
				temp->second->possOfSwitch++;
			}
		}

	}

	for (auto &it: valueModel)
	{
		it.second->possibility /= n; 

		double amount = 0;  // количество переходов в значениях
		for (auto &pair: it.second->nearestNodes)
		{
			amount += pair.second->possOfSwitch;


		}
		for (auto &pair : it.second->nearestNodes)
		{
			pair.second->possOfSwitch /= amount;
		}
	}
 
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
	double** connexion/*связи*/ = new double* [valueModel.size()];
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
		auto val = valueModel.find(array.find(i)->second)->second;
		//5 теперь в внутренем цикле:(фор ауто или обычный) ходим по всем ближайшим нодам у ноды которую мы взяли ранее
		for (size_t q = 0; q < array.size(); q++)  
		{
			//6 если взятая ближайшая нода совпадает с индексом нашего асоциативного массива записываем по индексу вероятность перехода к этой ноде 
			if (val->nearestNodes.find(array.find(i)->second) != val->nearestNodes.end())
			{
				connexion[i][q] = val->nearestNodes.find(array.find(i)->second)->second->possOfSwitch;
			}
			//if (val->nearestNodes.find(array.find(i)->second)->second->nearNode->value == array.find(i)->second) {}   //на карандаш жирным шрифтом 
	
			//в противном случае записываем 0 
			else
			{
				connexion[i][q] = 0;
			}
		}
		
	}
	return connexion;
}





