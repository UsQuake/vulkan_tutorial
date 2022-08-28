
#pragma once
#include "ObjectPool.h"
class ObjectHashMap {
public:
	ObjectUnorderedPool objects[8];
	int m_count = 0;
	void Add(Renderable object)
	{
		m_count++;
		objects[Hash(object.name[0])].Add(object);
	}
	void Delete(const char* name)
	{
		for (int i = 0; i < objects[Hash(name[0])].GetCount(); i++)
		{
			if (strcmp(name, objects[Hash(name[0])][i]->name) == 0)
			{
				
			}
		}
	}
	char Hash(char a)
	{
		return a - (a >> 3) << 3;
	}
	Renderable* operator[](const char* name)
	{
			
		for (int i = 0; i < objects[Hash(name[0])].GetCount(); i++)
		{
			if (strcmp(name, objects[Hash(name[0])][i]->name) == 0)
				return objects[Hash(name[0])][i];
		}
		return nullptr;

	}
	void Release()
	{
		for (int i = 0; i < 8; i++)
		{
			objects[i].Release();
		}
	}
};