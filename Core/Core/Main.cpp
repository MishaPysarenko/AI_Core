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

void main() 
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);

	std::fstream file("test.txt", std::ios::in);
	if (!file.is_open())
	{
		std::cout << "error open file\n";
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
	Model<std::fstream> test;
	auto oppWDataSet = [](std::fstream& DataSet)->TYPE* 
	{
		std::string data;
		std::list<std::string> tempdataset;
		size_t amountData = 0;

		while (std::getline(DataSet,data, ' '))
		{
			tempdataset.push_back(data);
			amountData++;
		}
		
		TYPE* resData = new TYPE[amountData + 1];
		resData[0].sizeData = amountData + 1;
		size_t i = 1;
		for (auto it: tempdataset)
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
	test.createModel(file, oppWDataSet);
	/*auto temp = test.retArrValue();
	auto temp1 = test.retMatrixConnexion();
	std::cout << "\t";*/

	std::list<std::string> prompt1;
	std::string temp3;
	while (std::getline(std::cin, temp3))
	{
		prompt1.push_back(temp3);

	}
	for (auto it: prompt1)
	{
		std::cout << it;
	}
	
	/*for (size_t i = 0; i < test.Size(); i++)
	{
		std::string str(
			static_cast<const char*>(temp[i].pData),
			temp[i].sizeData
		);
		std::cout << str << "\t";
	}

	for (size_t i = 0; i < test.Size(); i++)
	{
		std::string str(
			static_cast<const char*>(temp[i].pData),
			temp[i].sizeData
		);
		std::cout << str << ":\t\t";
		for (size_t q = 0; q < test.Size(); q++)
		{
			std::cout << temp1[i][q] << "\t";
		}
		std::cout << "\n";
	}
	std::cout << "\n\n\n";*/
	//TYPE prompt[] = {
	//{nullptr, 5, compare, defaultHash},
	//{const_cast<char*>("team"),     4, compare, defaultHash},
	//{const_cast<char*>("fortress"), 8, compare, defaultHash},
	//{const_cast<char*>("two"),      3, compare, defaultHash},
	//{const_cast<char*>("that"),     4, compare, defaultHash}
	//};
	//auto res = test.generationPrompt(prompt, 10);
	//for (size_t i = 0; i < 10; i++)
	//{
	//	std::string str(
	//		static_cast<const char*>(res[i].pData),
	//		res[i].sizeData
	//	);
	//	std::cout << str << "\t";
	//}
}






