#pragma once
#include <functional>
#include <list>

enum Callback_InsertMethod
{
	Callback_InsertMethod_PrependList = 0,
	Callback_InsertMethod_AppendList
};

template <typename T>
class TCallbackList
{
	typedef std::function<bool(const T&)> Type;

public:
	void AddCallbackHandler(const Type& callback, Callback_InsertMethod insertMethod = Callback_InsertMethod_AppendList)
	{
		switch (insertMethod)
		{
		case Callback_InsertMethod_PrependList:
			m_Callbacks.insert(m_Callbacks.begin(), callback);
			break;
			
		default:
		case Callback_InsertMethod_AppendList:
			m_Callbacks.insert(m_Callbacks.end(), callback);
			break;
		}
		
	}

	void RemoveCallbackHandler(const Type& callback)
	{
		m_Callbacks.remove(callback);
	}

	void FireCallbacks(T& param)
	{
		auto itr = m_Callbacks.begin(), end = m_Callbacks.end();
		while (itr != end && (*itr)(param) != true)
		{
			++itr;
		}
	}

protected:
	std::list<Type> m_Callbacks;
};