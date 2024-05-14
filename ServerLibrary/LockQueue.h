#pragma once

template<typename T>
class LockQueue
{
private:
	USE_LOCK;

public:
	void Push(T item)
	{
		EXCLUSIVE_LOCK;
		_items.push(item);
	}

	T Pop()
	{
		EXCLUSIVE_LOCK;
		return PopNoLock();
	}

	T PopNoLock()
	{
		if (_items.empty())
			return T();

		T ret = _items.front();
		_items.pop();
		return ret;
	}

	void PopAll(OUT vector<T>& items)
	{
		EXCLUSIVE_LOCK;
		while (T item = PopNoLock())
			items.push_back(item);
	}

	void Clear()
	{
		EXCLUSIVE_LOCK;
		_items = queue<T>();
	}

private:
	queue<T> _items;
};