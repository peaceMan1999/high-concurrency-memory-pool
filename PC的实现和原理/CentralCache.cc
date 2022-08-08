#define _CRT_SECURE_NO_WARNINGS 1
#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_cc;


Span* CentralCache::GetOneSpan(SpanList& list, size_t bytes) // 假如Span为空，获取Span
{
	// 先遍历链表看看有无span，有就拿
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_SpanFreeList != nullptr) // 有就拿走
		{
			return it;
		}
		else // 没有就下一个
		{
			it = it->_next;
		}
	}

	// 要先解开桶锁
	list.CC_mtx.unlock();
	// 走到这说明CC没有，开始问PC拿span，申请一块大内存，无组装过，纯净的，拿了需要组装，不过得加锁处理
	PageCache::GetInstance()->PC_mtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(bytes));
	PageCache::GetInstance()->PC_mtx.unlock();

	// 起始位置
	char* start = (char*)(span->_pageId << PAGESHIFT);
	size_t BigBytes = span->_n << PAGESHIFT; // 页数*8k
	char* end = start + BigBytes;  // 加上整个就算数组大小
	// 开始组装，把一块大内存组装成一个自由链表

	span->_SpanFreeList = start;
	void* tail = start;
	start += bytes;
	int i = 1;
	while (start < end)
	{
		NextObj(tail) = start;
		tail = start;
		start += bytes;
		i++;
	}
	cout << i << endl;

	// 头插进CC的byte上
	list.CC_mtx.lock();
	list.PushFront(span);

	return span;
}


size_t CentralCache::GetRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes) // 实际给TC的数量
{
	size_t index = SizeClass::Index(bytes);
	_CCSpanList[index].CC_mtx.lock(); // 遍历桶的时候加锁
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

	span->_usecount += actualNum;  // 拿走几个就+几个
	// ..还不够怎么办

	_CCSpanList[index].CC_mtx.unlock(); // 解锁

	return actualNum;
}

