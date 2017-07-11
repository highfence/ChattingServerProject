#pragma once
#include <vector>
#include <memory>
#include <deque>


namespace LogicLib
{
	template<class T>
	class ObjectPool
	{
	public :
		ObjectPool(const int poolSize);
		~ObjectPool();

		int GetTag();
		void ReleaseTag(const int tag);

		// Like stl containers...
		bool empty() const { return _pool.empty(); }
		size_t size() const { return _pool.size(); }
		int objNumber() const { return _activatedObjects; }
		T& at(const int tag) const { return &_pool.at(tag); }

	private :
		std::vector<T> _pool;
		std::deque<int> _poolIndex;
		int _activatedObjects = 0;

	};

	template<class T>
	inline ObjectPool<T>::ObjectPool(const int poolSize)
	{
		_pool.reserve(poolsize);
		_poolIndex.reserve(poolSize);

		for (int i = 0; i < poolSize; ++i)
		{
			_pool.emplace(new T());
			_poolIndex.push_back(i);
		}
	}

	template<class T>
	inline ObjectPool<T>::~ObjectPool()
	{
		_pool.clear();
		_pool.shrink_to_fit();
		_poolIndex.clear();
		_poolIndex.shrink_to_fit();
	}

	template<class T>
	inline int ObjectPool<T>::GetTag()
	{
		if (_poolIndex.empty())
		{
			return -1;
		}

		auto returnTag = _poolIndex.front();
		_poolIndex.pop_front();

		return returnTag;
	}

	template<class T>
	inline void ObjectPool<T>::ReleaseTag(const int tag)
	{
		_poolIndex.push_back(tag);
	}
}