#pragma once
#include "RenderableObject.h"

struct BulletInstance{
	Renderable ModelBullet;
	bool activate;
};
class BulletPool {
	unsigned int currentBulletIndex;
	BulletInstance m_pool[10];
 public:
	BulletPool()
	{

	}
	void Shoot()
	{

	}
	void ReturnToPool()
	{

	}
};