#define _CRT_SECURE_NO_WARNINGS 1
#include "PageCache.h"

PageCache PageCache::_PC;

Span* PageCache::NewSpan(size_t page) // 创建对应的span, k是页
{
	assert(page > 0 && page < MAXPAGES);

	// 先看该桶下有无span，有就给一个page的链表
	if (!_PCSpanList[page].Empty())
	{
		return _PCSpanList[page].PopFront();
	}

	// 没有就遍历后面的桶,，直到都没有
	for (int n = page + 1; n < MAXPAGES; n++)
	{
		if (!_PCSpanList[n].Empty()) // 说明有，就切分，例如要2，3 有就要切成 1 和 2
		{
			Span* nspan = _PCSpanList[n].PopFront(); // 弹出来切,剩余的再挂起
			Span* kspan = new Span; // k = n - page，我们要给CC的

			// 从nspan的头切一个下来
			kspan->_pageId = nspan->_pageId;
			kspan->_n = page;
			nspan->_pageId += page; // 往后退
			nspan->_n -= page;

			_PCSpanList[nspan->_n].PushFront(nspan);

			return kspan;
		}
	}

	// 没有更大的页了，得向堆申请空间

	Span* BigSpan = new Span;
	void* ptr = SystemAlloc(MAXPAGES - 1); // 申请 128 个页  128 * 8k 的大块内存

	BigSpan->_pageId = (PAGE_ID)ptr >> PAGESHIFT; // 除8k就算页号，具体看博客
	BigSpan->_n = MAXPAGES - 1; // 大块内存的页倍数

	_PCSpanList[BigSpan->_n].PushFront(BigSpan); // 插入到PC的链表上

	return NewSpan(page); // 递归调用一下自己，再切分
}