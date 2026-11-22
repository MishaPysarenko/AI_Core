#pragma once
#include"Node.h"
#include <unordered_map>

using valueChain = std::unordered_map<TYPE, std::shared_ptr<Node>>; //тип данных значение модели

class Model;

class FormalLogicEngine;

class Model
{
	friend class Model;// подружили :)
	friend class FormalLogicEngine;// +1 кореш
	valueChain valueModel;
public:
	Model() = default;
	Model(TYPE* dataSet, size_t sizeDataSet);
	Model(Model&A, Model& B);
	Model(Model& Template);
	void clear();
	void createModel(TYPE* dataSet,size_t sizeDataSet);
	size_t Size();
	TYPE* retArrValue();
	double** retMatrixConnexion();

	//посмотреть потом если будет проблема с памятью(оптимизация выдиления памяти)
	Model* operator + (Model& fusion);//сплюсовать и создать новую модель
	Model* operator - (Model& fusion);//отнять и создать новую модель

	void operator= (Model& copy);

	void operator += (Model& fusion);//сплюсовать и записать в текущею
	void operator -= (Model& fusion);//отнять и записать в текущею

	double operator == (Model& compare);//сравнение моделей
	double operator != (Model& compare);//сравнение моделей

	TYPE* generationPrompt(TYPE* prompt, size_t amountToken);

	void saveToFile(std::string fileName);

	void loadToFile(std::string fileName);
};
