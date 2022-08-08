#pragma once
#include "Common.h"

class ThreadCache
{
public:
	// 声明
	void* Allocate(size_t bytes); // 申请内存

	void Deallocate(void* ptr, size_t bytes); //释放内存

	void* GetFromCentalCache(size_t index, size_t bytes); // 不够辣！从中心获取内存

private:
	FreeList _TCFreeList[MAXBUCKETS]; // 管理_freeList自由链表，208个哈希桶，0~207;
};

// TLS解决线程间锁的问题
// 这个对象只有自己能看，其他线程不能看
static _declspec(thread) ThreadCache* pTSLThreadCache = nullptr;