#define _CRT_SECURE_NO_WARNINGS 1
#include "CentralCache.h"

CentralCache CentralCache::_cc;


Span* CentralCache::GetOneSpan(SpanList& list, size_t bytes) // 假如Span为空，获取Span
{
	return nullptr;
}


size_t CentralCache::GetRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes) // 实际给TC的数量
{
	size_t index = SizeClass::Index(bytes);
	_CCSpanList[index]._mtx.lock(); // 桶加锁
	// TODO
	Span* span = GetOneSpan(_CCSpanList[index], bytes); // 要拿内存，至少这个节点上有才行
	assert(span);
	assert(span->_SpanFreeList);

	start = span->_SpanFreeList;
	end = start; // end先等于start，然后再往后跑

	size_t i = 0, actualNum = 1;
	while (i < batchNum-1 && NextObj(end) != nullptr) // -1 是因为end
	{
		end = NextObj(end); // 往后走
		i++;
		actualNum++;
	}
	span->_SpanFreeList = NextObj(end); // 拿走后把剩下的第一个连上
	NextObj(end) = nullptr; // 把拿走的最后一个的_next置空

	// ..还不够怎么办

	_CCSpanList[index]._mtx.unlock(); // 解锁

	return actualNum;
}

