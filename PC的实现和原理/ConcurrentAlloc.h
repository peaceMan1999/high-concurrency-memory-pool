#pragma once
#include "Common.h"
#include "ThreadCache.h"

static void* ConcurrentAlloc(size_t bytes)
{
	if (pTSLThreadCache == nullptr)
	{	//说明还没创建，申请内存，通过TSL无锁地获取自己的ThreadCache对象
		pTSLThreadCache = new ThreadCache;
	}

	//cout << std::this_thread::get_id() << " : " << pTSLThreadCache << endl;

	return pTSLThreadCache->Allocate(bytes); // 调用自己的成员函数申请空间
}

static void ConcurrentFree(void* ptr, size_t bytes)
{
	assert(pTSLThreadCache); // 判断是否申请

	pTSLThreadCache->Deallocate(ptr, bytes);
}
