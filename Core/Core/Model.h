#pragma once
#include"Node.h"
#include <unordered_map>

using valueChain = std::unordered_map<TYPE, std::shared_ptr<Node>>; //тип данных значение модели

template <typename DATA_SET>
class Model;

template <typename DATA_SET>
class Model
{
	friend class Model;// подружили :)
	valueChain valueModel;
public:
	Model() = default;
	Model(DATA_SET dataSet, TYPE*(*operationsWDataSet)(DATA_SET dataSet));
	Model(Model<DATA_SET>&A, Model<DATA_SET>& B);
	Model(Model<DATA_SET>& Template);
	void clear();
	void createModel(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet));
	size_t Size();
	TYPE* retArrValue();
	double** retMatrixСonnexion(); 
	//посмотреть потом если будет проблема с памятью(оптимизация выдиления памяти)
	Model* operator + (Model& fusion);//сплюсовать и создать новую модель
	Model* operator - (Model& fusion);//отнять и создать новую модель

	void operator += (Model& fusion);//сплюсовать и записать в текущею
	void operator -= (Model& fusion);//отнять и записать в текущею

	double operator == (Model& compare);//сравнение моделей
	double operator != (Model& compare);//сравнение моделей

	TYPE* generationPrompt(TYPE* prompt, size_t amountToken);
};

