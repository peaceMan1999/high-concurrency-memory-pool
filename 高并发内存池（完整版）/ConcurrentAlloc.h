#pragma once
#include "Common.h"
#include "PageCache.h"
#include "ThreadCache.h"

static void* ConcurrentAlloc(size_t bytes)
{
	if (bytes > MAXBYTES)
	{
		// 直接在堆上申请
		size_t alignSize = SizeClass::RoundUp(bytes);
		size_t page = alignSize >> PAGESHIFT; // 页数

		PageCache::GetInstance()->PC_mtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(page);
		span->_objectSize = bytes;
		PageCache::GetInstance()->PC_mtx.unlock();

		void* ptr = (void*)(span->_pageId << PAGESHIFT);
		return ptr;
	}
	else
	{
		if (pTSLThreadCache == nullptr)
		{	//说明还没创建，申请内存，通过TSL无锁地获取自己的ThreadCache对象
			static ObjectPool<ThreadCache> TCPool;
			pTSLThreadCache = TCPool.New();
		}
		//cout << std::this_thread::get_id() << " : " << pTSLThreadCache << endl;

		return pTSLThreadCache->Allocate(bytes); // 调用自己的成员函数申请空间
	}
}

static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t bytes = span->_objectSize;

	if (bytes > MAXBYTES)
	{
		PageCache::GetInstance()->PC_mtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPC(span);
		PageCache::GetInstance()->PC_mtx.unlock();
	}
	else
	{
		assert(pTSLThreadCache); // 判断是否申请
		pTSLThreadCache->Deallocate(ptr, bytes);
	}
}
