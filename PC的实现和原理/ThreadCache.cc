#define _CRT_SECURE_NO_WARNINGS 1
#include "ThreadCache.h"
#include "CentralCache.h"


void* ThreadCache::GetFromCentalCache(size_t index, size_t bytes) // 不够辣！从中心获取内存
{
	// 慢反馈调节
	size_t batchNum = min(_TCFreeList[index].MaxSize(), SizeClass::NumMoveSize(bytes));// 处理大小
	// 从慢到快
	if (batchNum == _TCFreeList[index].MaxSize())
	{
		_TCFreeList[index].MaxSize() += 1; // 慢->快		
	}
	void* start = nullptr;
	void* end = nullptr;
	// 获取实际大小
	size_t actualNum = CentralCache::GetInstance()->GetRangeObj(start, end, batchNum, bytes);
	assert(actualNum > 0);

	if (actualNum == 1) // 返回头
	{
		assert(start == end);
		return start;
	}
	else
	{
		_TCFreeList[index].PushRange(NextObj(start), end); // 多的先放着，先给一个
		return start;
	}
}

void* ThreadCache::Allocate(size_t bytes) // 申请内存
{
	assert(bytes <= MAXBYTES);
	size_t alignSize = SizeClass::RoundUp(bytes); // 获取实际对齐bytes
	size_t index = SizeClass::Index(bytes);  // 哪个桶

	if (!_TCFreeList[index].Empty())
	{
		// 不为空就获取内存
		return _TCFreeList[index].Pop();
	}
	else 
	{
		// 空就往中心申请内存
		return GetFromCentalCache(index, alignSize);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t bytes) //释放内存
{
	assert(ptr);
	assert(bytes <= MAXBYTES);

	size_t index = SizeClass::Index(bytes);  // 哪个桶

	// 添加回自由链表
	_TCFreeList[index].Push(ptr);
}


