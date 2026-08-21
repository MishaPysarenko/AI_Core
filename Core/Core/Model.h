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
	Model(valueChain& A, valueChain& B);
	void clear();
	void createModel(DATA_SET dataSet, TYPE* (*operationsWDataSet)(DATA_SET dataSet));
	size_t Size();
	TYPE* retArrValue();
	double** retMatrixСonnexion(); 

	Model* operator + (Model& fusion);//сплюсовать и создать новую модель
	Model* operator - (Model& fusion);//отнять и создать новую модель

	void operator += (Model& fusion);//сплюсовать и записать в текущею
	void operator -= (Model& fusion);//отнять и записать в текущею

	double operator == (Model& compare);//сравнение моделей
	double operator != (Model& compare);//сравнение моделей

	TYPE* generationPrompt(TYPE* prompt, size_t amountToken);
};

