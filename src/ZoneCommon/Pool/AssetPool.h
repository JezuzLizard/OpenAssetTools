#pragma once

#include "XAssetInfo.h"
#include <string>
#include <map>

template<typename T>
class AssetPool
{
public:
    std::map<std::string, XAssetInfo<T>*> m_asset_lookup;

    class Iterator
    {
        typename std::map<std::string, XAssetInfo<T>*>::iterator m_iterator;

    public:
        explicit Iterator(typename std::map<std::string, XAssetInfo<T>*>::iterator i)
        {
            m_iterator = i;
        }

        bool operator!=(Iterator rhs)
        {
            return m_iterator != rhs.m_iterator;
        }

        XAssetInfo<T>* operator*()
        {
            return m_iterator.operator*().second;
        }

        void operator++()
        {
            ++m_iterator;
        }
    };

    AssetPool()
    {
        m_asset_lookup = std::map<std::string, XAssetInfo<T>*>();
    }

    virtual ~AssetPool() = default;

    virtual XAssetInfo<T>* AddAsset(std::string name, T* asset, std::vector<std::string>& scriptStrings, std::vector<XAssetInfoGeneric*>& dependencies) = 0;

    XAssetInfo<T>* GetAsset(const std::string& name)
    {
        auto foundAsset = m_asset_lookup.find(name);

        if(foundAsset == m_asset_lookup.end())
            return nullptr;

        return foundAsset->second;
    }

    Iterator begin()
    {
        return Iterator(m_asset_lookup.begin());
    }

    Iterator end()
    {
        return Iterator(m_asset_lookup.end());
    }
};