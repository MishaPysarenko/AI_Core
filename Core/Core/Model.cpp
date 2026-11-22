#include "Model.h"
#include <fstream>
#include <iostream>
#include <cstdint>

Model::Model(TYPE* dataSet, size_t sizeDataSet)
{
	createModel(dataSet, sizeDataSet);
}

Model::Model(Model& A, Model& B)
{
	Model* temp = A + B;
	valueModel = temp->valueModel;
	delete temp;
}

Model::Model(Model& Template)
{
	valueModel = Template.valueModel;
}

void Model::createModel(TYPE* dataSet, size_t sizeDataSet)
{
	TYPE* arrayValue = dataSet;
	size_t n = sizeDataSet;
	if (arrayValue == nullptr || n == 0)
		return;

	for (size_t i = 0; i < n - 1; i++)
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
			if (valueModel.find(arrayValue[i + 1]) == valueModel.end())
			{
				std::shared_ptr<Node> temp = std::make_shared<Node>();
				temp->value = arrayValue[i+1];
				temp->possibility = 1;
				valueModel[arrayValue[i+1]] = temp;
			}
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
		it.second->possibility /= (n - 1); 

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

double** Model::retMatrixConnexion()
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
			if (val->nearestNodes.find(array.find(q)->second) != val->nearestNodes.end())
			{
				connexion[i][q] = val->nearestNodes.find(array.find(q)->second)->second->possOfSwitch;
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

// Объединение двух моделей
Model* Model::operator+(Model& fusion)
{
	Model* result = new Model();

	// Создаём независимые узлы из первой модели
	for (const auto& [key, node] : valueModel)
	{
		auto copy = std::make_shared<Node>();
		copy->value = node->value;
		copy->possibility = node->possibility;
		result->valueModel[key] = copy;
	}

	// Добавляем состояния второй модели
	for (const auto& [key, node] : fusion.valueModel)
	{
		auto it = result->valueModel.find(key);

		if (it == result->valueModel.end())
		{
			auto copy = std::make_shared<Node>();
			copy->value = node->value;
			copy->possibility = node->possibility;
			result->valueModel[key] = copy;
		}
		else
		{
			it->second->possibility =
				(it->second->possibility +
					node->possibility) / 2.0;
		}
	}

	// Переносим связи первой модели
	for (const auto& [key, node] : valueModel)
	{
		auto current = result->valueModel.at(key);

		for (const auto& [nearKey, pair] : node->nearestNodes)
		{
			auto target = result->valueModel.find(nearKey);

			if (target == result->valueModel.end())
				continue;

			auto newPair = std::make_shared<Pair>();
			newPair->nearNode = target->second;
			newPair->possOfSwitch = pair->possOfSwitch;

			current->nearestNodes[nearKey] = newPair;
		}
	}

	// Объединяем связи второй модели
	for (const auto& [key, node] : fusion.valueModel)
	{
		auto current = result->valueModel.at(key);

		for (const auto& [nearKey, pair] : node->nearestNodes)
		{
			auto target = result->valueModel.find(nearKey);

			if (target == result->valueModel.end())
				continue;

			auto existing = current->nearestNodes.find(nearKey);

			if (existing == current->nearestNodes.end())
			{
				auto newPair = std::make_shared<Pair>();
				newPair->nearNode = target->second;
				newPair->possOfSwitch = pair->possOfSwitch;

				current->nearestNodes[nearKey] = newPair;
			}
			else
			{
				existing->second->possOfSwitch =
					(existing->second->possOfSwitch +
						pair->possOfSwitch) / 2.0;
			}
		}
	}

	return result;
}

// Вычитание двух моделей
Model* Model::operator-(Model& fusion)
{
	Model* result = new Model();

	// Копируем состояния первой модели
	for (const auto& [key, node] : valueModel)
	{
		auto copy = std::make_shared<Node>();
		copy->value = node->value;
		copy->possibility = node->possibility;

		result->valueModel[key] = copy;
	}

	// Вычитаем вероятности состояний
	for (const auto& [key, node] : fusion.valueModel)
	{
		auto it = result->valueModel.find(key);

		if (it == result->valueModel.end())
			continue;

		it->second->possibility = std::max(
			0.0,
			2.0 * it->second->possibility -
			node->possibility
		);
	}

	// Копируем связи первой модели и вычитаем совпадающие
	for (const auto& [key, node] : valueModel)
	{
		auto current = result->valueModel.at(key);

		auto fusionNode = fusion.valueModel.find(key);

		for (const auto& [nearKey, pair] : node->nearestNodes)
		{
			double probability = pair->possOfSwitch;

			if (fusionNode != fusion.valueModel.end())
			{
				auto otherPair =
					fusionNode->second->nearestNodes.find(nearKey);

				if (otherPair !=
					fusionNode->second->nearestNodes.end())
				{
					probability = std::max(
						0.0,
						2.0 * probability -
						otherPair->second->possOfSwitch
					);
				}
			}

			if (probability <= 0.0)
				continue;

			auto target = result->valueModel.find(nearKey);

			if (target == result->valueModel.end())
				continue;

			auto newPair = std::make_shared<Pair>();
			newPair->nearNode = target->second;
			newPair->possOfSwitch = probability;

			current->nearestNodes[nearKey] = newPair;
		}
	}

	return result;
}

void Model::operator=(Model& copy)
{
	valueModel = copy.valueModel;
}

// Объединение с текущей моделью
void Model::operator+=(Model& fusion)
{
	// Сначала создаём независимую результирующую модель
	std::unique_ptr<Model> result((*this) + fusion);

	// Переносим новую структуру
	valueModel = std::move(result->valueModel);
}

// Вычитание из текущей модели
void Model::operator-=(Model& fusion)
{
	std::unique_ptr<Model> result((*this) - fusion);

	valueModel = std::move(result->valueModel);
}

double Model::operator==(Model& compare)
{
	double equalElements = 0;

	for (auto it : compare.valueModel)
	{
		if (valueModel.find(it.first) != valueModel.end())
		{
			equalElements++;
		}
	}
	if (equalElements == 0)
	{
		return 0;
	}
	return valueModel.size() / equalElements;
}

double Model::operator!=(Model& compare)
{
	double equalElements = 0;

	for (auto it : compare.valueModel)
	{
		if (valueModel.find(it.first) == valueModel.end())
		{
			equalElements++;
		}
	}
	if (equalElements == 0)
	{
		return 0;
	}
	return valueModel.size() / equalElements;
}

TYPE* Model::generationPrompt(TYPE* prompt, size_t amountToken)
{
	size_t amountSizePrompt = prompt[0].sizeData;
	double avarageValue = 0;
	double minDiff = 1;

	std::list<TYPE> tempAVG;
	TYPE min{};

	TYPE* res = new TYPE[amountToken];

	for (size_t i = 1; i < amountSizePrompt; i++)
	{
		auto it = valueModel.find(prompt[i]);

		if (it != valueModel.end())
		{
			avarageValue += it->second->possibility;
			tempAVG.push_back(prompt[i]);
		}
	}

	if (amountSizePrompt > 1)
		avarageValue /= (amountSizePrompt - 1);

	auto findNear = [avarageValue](const auto& a) -> TYPE
		{
			TYPE nearest{};
			double minDiff = std::numeric_limits<double>::max();

			for (const auto& it : a)
			{
				double diff = std::abs(
					avarageValue - it.second->possibility 
				);

				if (diff < minDiff)
				{
					minDiff = diff;
					nearest = it.first;
				}
			}

			return nearest;
		};


	auto findNear1 = [avarageValue](const auto& a) -> TYPE
		{
			TYPE nearest{};
			double minDiff = std::numeric_limits<double>::max();

			for (const auto& it : a)
			{
				double diff = std::abs(
					avarageValue - it.second->nearNode->possibility
				);

				if (diff < minDiff)
				{
					minDiff = diff;
					nearest = it.first;
				}
			}

			return nearest;
		};

	min = findNear(valueModel);

	for (size_t i = 0; i < amountToken; i++)
	{
		res[i] = min;

		auto it = valueModel.find(min);

		if (it == valueModel.end())
			break;

		const auto& nearestNodes = it->second->nearestNodes;

		if (nearestNodes.empty())
			break;

		if (nearestNodes.size() == 1)
		{
			min = nearestNodes.begin()->first;
		}
		else
		{
			min = findNear1(nearestNodes);
		}
	}

	return res;
}

void Model::saveToFile(std::string fileName)
{
	//std::fstream file(fileName + ".bin", std::ios::out | std::ios::binary);

	//size_t memoryUsed;

	//file.write(reinterpret_cast<const char*>(&valueModel.size()), sizeof(valueModel.size()));
	//
	//for (auto it: valueModel)
	//{
	//	memoryUsed = sizeof(it->second->value);
	//	file.write(reinterpret_cast<const char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.write(reinterpret_cast<const char*>(&it->second->value), sizeof(it->second->value)); 

	//	memoryUsed = sizeof(it->second->possibility);
	//	file.write(reinterpret_cast<const char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.write(reinterpret_cast<const char*>(&it->second->possibility), sizeof(it->second->possibility));

	//}	

	//for (auto it : valueModel)
	//{
	//	memoryUsed = sizeof(it->second->value);
	//	file.write(reinterpret_cast<const char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.write(reinterpret_cast<const char*>(&it->second->value), sizeof(it->second->value));

	//	memoryUsed = sizeof(it->second->nearestNodes.size());
	//	file.write(reinterpret_cast<const char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.write(reinterpret_cast<const char*>(&it->second->nearestNodes.size()), sizeof(it->second->nearestNodes.size()));

	//	for (auto pair : it->second->nearestNodes)
	//	{
	//		memoryUsed = sizeof(pair->second->nearNode->value);
	//		file.write(reinterpret_cast<const char*>(&memoryUsed), sizeof(memoryUsed));
	//		file.write(reinterpret_cast<const char*>(&pair->second->nearNode->value), sizeof(pair->second->nearNode->value));

	//		memoryUsed = sizeof(pair->second->possOfSwitch);
	//		file.write(reinterpret_cast<const char*>(&memoryUsed), sizeof(memoryUsed));
	//		file.write(reinterpret_cast<const char*>(&pair->second->possOfSwitch), sizeof(pair->second->possOfSwitch));

	//	}
	//}
}

void Model::loadToFile(std::string fileName)
{
	//std::ifstream file(fileName, std::iostream::binary);
	//valueModel.clear();
	//
	//size_t memoryUsed;

	//constexpr std::size_t BLOCK_SIZE = 1024;

	//std::array<uint8_t, BLOCK_SIZE> buffer;

	//TYPE tempValue;
	//double tempPoss;
	//size_t tempAmountNearNodes;
	//size_t tempNearNode;
	//size_t tempAmountValue;
	//double tempPossOfSwitch;

	//file.read(reinterpret_cast<char*>(&tempAmountValue), sizeof(tempAmountValue));

	//for (size_t i = 0; i < tempAmountValue; i++)
	//{
	//	file.read(reinterpret_cast<char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.read(reinterpret_cast<char*>(&tempValue), memoryUsed);

	//	file.read(reinterpret_cast<char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.read(reinterpret_cast<char*>(&tempPoss), sizeof(tempPoss));

	//	valueModel[tempValue] = { tempValue,tempPoss,{} };
	//}


	//while (file)
	//{
	//	file.read(reinterpret_cast<char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.read(reinterpret_cast<char*>(&tempValue), memoryUsed);

	//	file.read(reinterpret_cast<char*>(&memoryUsed), sizeof(memoryUsed));
	//	file.read(reinterpret_cast<char*>(&tempAmountValue), sizeof(tempAmountValue));

	//	auto tempNode = valueModel.find(tempValue)->second;

	//	for (size_t i = 0; i < tempAmountValue; i++)
	//	{
	//		file.read(reinterpret_cast<char*>(&memoryUsed), sizeof(memoryUsed));
	//		file.read(reinterpret_cast<char*>(&tempValue), memoryUsed);

	//		file.read(reinterpret_cast<char*>(&memoryUsed), sizeof(memoryUsed));
	//		file.read(reinterpret_cast<char*>(&tempPossOfSwitch), sizeof(tempPossOfSwitch));

	//		tempNode->nearestNodes[tempValue] = { valueModel.find(tempValue), tempPossOfSwitch };
	//	}
	//}
}

