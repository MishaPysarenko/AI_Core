#include "Model.h"
#include <fstream>
#include <iostream>
#include <list>

void main() 
{
	std::fstream file("test.txt", std::ios::in);
	Model<std::fstream> test;
	auto oppWDataSet = [](std::fstream& DataSet)->TYPE* 
	{
		std::string data;
		std::list<std::string> tempdataset;
		size_t amountData = 0;

		while (std::getline(DataSet,data))
		{
			tempdataset.push_back(data);
			amountData++;
		}
		
		TYPE* resData = new TYPE[amountData + 1];
		resData[0].sizeData = amountData + 1;
		size_t i = 1;
		for (auto it: tempdataset)
		{
			resData[i].pData = it.c_str();
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
			i++;

		}
		return resData;

	};
	test.createModel(file, oppWDataSet);
	auto temp = test.retArrValue();
	auto temp1 = test.retMatrixConnexion();

	for (size_t i = 0; i < test.Size(); i++)
	{
		for (size_t q = 0; q < test.Size(); q++)
		{
			std::cout << temp1[i][q] << "\n";
		}
	}
}






