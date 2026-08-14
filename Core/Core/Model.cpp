#include "Model.h"
template <typename DATA_SET>
Model<DATA_SET>::Model(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet))
{
	createModel(dataSet, operationsWDataSet);
}

template<typename DATA_SET>
Model<DATA_SET>::Model(valueChain& A, valueChain& B)
{
}

template <typename DATA_SET>
void Model<DATA_SET>::createModel(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet))
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

template <typename DATA_SET>
size_t Model<DATA_SET>::Size()
{
	return valueModel.size();
}

template <typename DATA_SET>
void Model<DATA_SET>::clear() 
{
	valueModel.clear();
}

template <typename DATA_SET>
TYPE* Model<DATA_SET>::retArrValue()
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

template <typename DATA_SET>
double** Model<DATA_SET>::retMatrixСonnexion()
{
	//1 создание асоциативного массива с определенным порядком значений и их индексов 
	double** connexion/*связи*/ = new double* [valueModel.size()];
	TYPE* tempARR = retArrValue();
	std::unordered_map<size_t ,TYPE> array;
	for (size_t i = 0; i < valueModel.size(); i++)
		array[i] = tempARR[i];
	delete[] tempARR;

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

template<typename DATA_SET>
Model<DATA_SET>* Model<DATA_SET>::operator+(Model& fusion)
{
	return Model();
}

template<typename DATA_SET>
Model<DATA_SET>* Model<DATA_SET>::operator-(Model& fusion)
{
	return Model();
}

template<typename DATA_SET>
void Model<DATA_SET>::operator+=(Model& fusion)
{
	for (auto it : fusion->valueModel)
	{
		if (valueModel.find(it->first) == valueModel.end())
		{
			valueModel[it->first] = it->second;
		}
	}
	//дз операторы сравнения 
	//суть: перебрать все ноды и если все совпали то =
	//если не совпали то !=
	for (auto &it : valueModel)
	{
		if (fusion->valueModel.find(it.first) != fusion.valueModel.end())
		{
			it.second->possibility += fusion.valueModel.find(it.first)->second->possibility;
			it.second->possibility /= 2;
		}
	}

	//добавить недостающие связи
	// у связей коотрые былa веротность перехода сделать среднее ареф.
	for (auto &it : valueModel)
	{
		for (auto pair : it.second->nearestNodes)
		{
			it.second->nearestNodes.find(it.first)->second->possOfSwitch += fusion->valueModel.find(it.first)->second->nearestNodes.find(it.first)->second->possOfSwitch;
			it.second->nearestNodes.find(it.first)->second->possOfSwitch /= 2;
		}

	}
}

template<typename DATA_SET>
void Model<DATA_SET>::operator-=(Model& fusion)
{
	fusion->valueModel;

}

template<typename DATA_SET>
bool Model<DATA_SET>::operator==(Model& compare)
{
	return false;
}

template<typename DATA_SET>
bool Model<DATA_SET>::operator!=(Model& compare)
{
	return false;
}

