#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <cstddef>

struct TYPE
{
private:
    std::shared_ptr<const void> pData;

    std::type_index dataType{ typeid(void) };

    std::function<bool(const void*, const void*)> compareFunc;
    std::function<size_t(const void*)> hashFunc;

    size_t sizeData = 0;

    template<typename T>
    void set(T value)
    {
        auto data = std::make_shared<T>(std::move(value));

        pData = data;
        dataType = std::type_index(typeid(T));
        sizeData = sizeof(T);

        compareFunc = [](const void* a, const void* b)
            {
                return *static_cast<const T*>(a) ==
                    *static_cast<const T*>(b);
            };

        hashFunc = [](const void* data)
            {
                return std::hash<T>{}(
                    *static_cast<const T*>(data)
                    );
            };
    }

public:

    TYPE() = default;

    size_t size() const noexcept
    {
        return sizeData;
    }

    template<typename T>
    explicit TYPE(T value){set<T>(std::move(value));}

    template<typename T>
    const T& get() const
    {
        if (dataType != std::type_index(typeid(T)))
            throw std::bad_cast();

        if (!pData)
            throw std::runtime_error("TYPE is empty");

        return *static_cast<const T*>(pData.get());
    }

    bool operator==(const TYPE& other) const
    {
        if (dataType != other.dataType)
            return false;

        if (!pData || !other.pData)
            return !pData && !other.pData;

        return compareFunc(pData.get(), other.pData.get());
    }

    bool operator!=(const TYPE& other) const
    {
        return !(*this == other);
    }

    size_t hash() const
    {
        size_t typeHash = dataType.hash_code();

        if (!pData)
            return typeHash;

        size_t valueHash = hashFunc(pData.get());

        return typeHash ^ (
            valueHash + 0x9e3779b9 +
            (typeHash << 6) +
            (typeHash >> 2)
            );
    }

    std::type_index type() const
    {
        return dataType;
    }

    bool empty() const
    {
        return !pData;
    }
};

namespace std
{
    template<>
    struct hash<TYPE>
    {
        size_t operator()(const TYPE& value) const
        {
            return value.hash();
        }
    };
}

struct Node;

struct Pair
{
	std::shared_ptr<Node> nearNode;
	double possOfSwitch; // вероятность перехода 
};

struct Node
{
	TYPE value;
	double possibility; 
	std::unordered_map<TYPE, std::shared_ptr<Pair>> nearestNodes;

};