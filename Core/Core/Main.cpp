#include "Model.h"
#include <fstream>
#include <iostream>
#include <list>
#include <windows.h> 

size_t defaultHash(TYPE value)
{
	const unsigned char* data =
		static_cast<const unsigned char*>(value.pData);

	size_t hash = 14695981039346656037ull;

	for (size_t i = 0; i < value.sizeData; i++)
	{
		hash ^= data[i];
		hash *= 1099511628211ull;
	}

	return hash;
}

bool (*compare)(TYPE, TYPE) = [](TYPE comparedA, TYPE comparedB)
	{
		if (comparedB.sizeData != comparedA.sizeData)
		{
			return false;
		}
		if (comparedA.pData != comparedB.pData)
		{
			return false;
		}return true;

	};

auto oppWDataSet = [](std::fstream& DataSet)->TYPE*
	{
		std::string data;
		std::list<std::string> tempdataset;
		size_t amountData = 0;

		while (std::getline(DataSet, data, ' '))
		{
			tempdataset.push_back(data);
			amountData++;
		}

		TYPE* resData = new TYPE[amountData + 1];
		resData[0].sizeData = amountData + 1;
		size_t i = 1;
		for (auto it : tempdataset)
		{
			char* tempVal = new char[it.size()];
			for (size_t i = 0; i < it.size(); i++)
				tempVal[i] = it[i];

			resData[i].pData = tempVal;
			resData[i].sizeData = it.size();
			resData[i].compare = [](TYPE comparedA, TYPE comparedB)
				{
					if (comparedB.sizeData != comparedA.sizeData)
					{
						return false;
					}
					if (comparedA.pData != comparedB.pData)
					{
						return false;
					}return true;

				};
			resData[i].hash = defaultHash;
			i++;

		}
		return resData;

	};

void showListConnection(TYPE* val, double** connection, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		std::string from(
			static_cast<const char*>(val[i].pData),
			val[i].sizeData
		);

		for (size_t j = 0; j < size; j++)
		{
			if (connection[i][j] != 0.0)
			{
				std::string to(
					static_cast<const char*>(val[j].pData),
					val[j].sizeData
				);

				std::cout << from
					<< " -> "
					<< to
					<< " : "
					<< connection[i][j]
					<< std::endl;
			}
		}
	}
}

void showChains(TYPE* val, double** connection,size_t size,size_t current,std::vector<bool>& visited,std::vector<size_t>& path)
{
	visited[current] = true;
	path.push_back(current);

	bool hasNext = false;

	for (size_t j = 0; j < size; j++)
	{
		if (connection[current][j] == 0.0)
			continue;

		if (visited[j])
		{
			// Цепочка завершилась повторением
			for (size_t k : path)
			{
				std::string str(
					static_cast<const char*>(val[k].pData),
					val[k].sizeData
				);
				std::cout << str << " -> ";
			}

			std::string str(
				static_cast<const char*>(val[j].pData),
				val[j].sizeData
			);

			std::cout << str << " [REPEAT]\n";
			continue;
		}

		hasNext = true;

		showChains(val, connection, size, j, visited, path);
	}

	if (!hasNext)
	{
		bool hasConnections = false;

		for (size_t j = 0; j < size; j++)
			if (connection[current][j] != 0.0)
				hasConnections = true;

		if (!hasConnections)
		{
			for (size_t k = 0; k < path.size(); k++)
			{
				std::string str(
					static_cast<const char*>(val[path[k]].pData),
					val[path[k]].sizeData
				);

				std::cout << str;

				if (k + 1 < path.size())
					std::cout << " -> ";
			}

			std::cout << " [END]\n";
		}
	}

	path.pop_back();
	visited[current] = false;
}

void showAllChains(TYPE* val, double** connection, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		std::vector<bool> visited(size, false);
		std::vector<size_t> path;

		showChains(
			val,
			connection,
			size,
			i,
			visited,
			path
		);
	}
}

void main() 
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);

	std::fstream file("test_en.txt", std::ios::in);
	if (!file.is_open())
	{
		std::cout << "error open file\n";
	}

	Model<std::fstream> test;
	
	test.createModel(file, oppWDataSet);
	/*
	std::cout << "\nConnection\n";

	showListConnection(test.retArrValue(), test.retMatrixConnexion(), test.Size());

	std::vector<bool> visited(test.Size(), false);
	std::vector<size_t> path;

	
	std::cout << "\nChains\n";

	showAllChains(test.retArrValue(), test.retMatrixConnexion(), test.Size());
	*/

	/*
	std::list<std::string> prompt1;
	std::string temp3,temp4;
	std::getline(std::cin, temp3);
	
	for (size_t i = 0; i < temp3.size(); i++)
	{
		if (temp3[i] != ' ')
		{
			temp4.push_back(temp3[i]);
		}
		else
		{
			prompt1.push_back(temp4);
			temp4.clear();
		}
	}
	
	prompt1.push_back(temp4);
	
	TYPE* Prompt = new TYPE[prompt1.size()+1];
	Prompt[0] = { nullptr, prompt1.size() + 1, compare, defaultHash };

	size_t index = 1;

	for (auto str: prompt1)
	{
		Prompt[index] = { const_cast<char*> (str.c_str()), str.size(), compare, defaultHash };
	}

	
	auto res = test.generationPrompt(Prompt, prompt1.size() + 1);

	for (size_t i = 0; i < prompt1.size() + 1; i++)
	{
		std::string str(
					static_cast<const char*>(res[i].pData),
					res[i].sizeData	);
		std::cout<< str << " ";
	}

	delete[] res;
	*/
}






