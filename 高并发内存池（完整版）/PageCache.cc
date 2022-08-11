#define _CRT_SECURE_NO_WARNINGS 1
#include "PageCache.h"

PageCache PageCache::_PC;

void PageCache::ReleaseSpanToPC(Span* span) // 合并内存
{
	if (span->_n > MAXPAGES - 1)
	{
		// 直接还给堆
		SystemFree((void*)(span->_pageId << PAGESHIFT));
		//delete span;
		_SpanPool.Delete(span);

		return;
	}

	while (1)
	{
		// 先找前面相邻的页的尾
		
		// 没有页号，不要合并了 -- 改良版本
		PAGE_ID previd = span->_pageId - 1;
		auto ret = (Span*)_IdSpanMap.get(previd);
		if (ret == nullptr)
		{
			break; // 没有就break
		}
		//auto ret = _IdSpanMap.find(previd);
		//if (ret == _IdSpanMap.end())
		//{
		//	break; // 没有就break
		//}

		Span* prevSpan = ret;
		if (prevSpan->_IsUse) // == true
		{
			break; // 在使用
		}

		if (prevSpan->_n + span->_n > MAXPAGES - 1)
		{
			break; // 页数过大，128
		}
		
		// 向前合并
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_PCSpanList[span->_n].Erase(prevSpan);
		//delete prevSpan;
		_SpanPool.Delete(prevSpan);

	}

	// 向后合并
	while (1)
	{
		// 先找后面相邻的页的头
		//PAGE_ID nextid = span->_pageId + span->_n;
		//auto ret = _IdSpanMap.find(nextid);
		//if (ret == _IdSpanMap.end())
		//{
		//	break; // 没有就break
		//}

		PAGE_ID previd = span->_pageId - 1;
		auto ret = (Span*)_IdSpanMap.get(previd);
		if (ret == nullptr)
		{
			break; // 没有就break
		}
		Span* nextSpan = ret;
		if (nextSpan->_IsUse)
		{
			break; // 在使用
		}

		if (nextSpan->_n + span->_n > MAXPAGES - 1)
		{
			break; // 页数过大，128
		}

		// 向后合并
		span->_n += nextSpan->_n;

		_PCSpanList[span->_n].Erase(nextSpan);
		// delete nextSpan;
		_SpanPool.Delete(nextSpan);
	}
	// 合并完后挂起
	_PCSpanList[span->_n].PushFront(span);
	span->_IsUse = false;
	//_IdSpanMap[span->_pageId] = span;
	//_IdSpanMap[span->_pageId + span->_n - 1] = span;

	_IdSpanMap.set(span->_pageId, span);
	_IdSpanMap.set(span->_pageId + span->_n - 1, span);

}

Span* PageCache::MapObjectToSpan(void* obj) // 获取从对象到span的对应关系
{
	PAGE_ID id = (PAGE_ID)obj >> PAGESHIFT; // 除8k，要转为10进制

	Span* span = (Span*)_IdSpanMap.get(id);

	//std::unique_lock<std::mutex> lock(PC_mtx); // 访问也要加锁

	//auto ret = _IdSpanMap.find(id);
	//if (ret != _IdSpanMap.end())
	//{
	//	return ret->second; // 找到返回 
	//}
	//else
	//{
	//	assert(false);
	//	return nullptr;
	//	// 不可能找不到;
	//}
	return span;
}

Span* PageCache::NewSpan(size_t page) // 创建对应的span, k是页
{
	assert(page > 0);
	
	if (page > MAXPAGES-1)
	{
		// 堆申请
		void* ptr = SystemAlloc(page);
		Span* span = _SpanPool.New();

		span->_pageId = (PAGE_ID)ptr >> PAGESHIFT;
		span->_n = page;

		//_IdSpanMap[span->_pageId] = span;
		_IdSpanMap.set(span->_pageId, span);

		return span;
	}

	// 先看该桶下有无span，有就给一个page的链表
	if (!_PCSpanList[page].Empty())
	{
		Span* kspan = _PCSpanList[page].PopFront();
		// 用的就建立每块映射
		for (PAGE_ID i = 0; i < kspan->_n; i++)
		{
			//_IdSpanMap[kspan->_pageId + i] = kspan;
			_IdSpanMap.set(kspan->_pageId + i, kspan);

		}
		return kspan;
	}

	// 没有就遍历后面的桶,，直到都没有
	for (int n = page + 1; n < MAXPAGES; n++)
	{
		if (!_PCSpanList[n].Empty()) // 说明有，就切分，例如要2，3 有就要切成 1 和 2
		{
			Span* nspan = _PCSpanList[n].PopFront(); // 弹出来切,剩余的再挂起
			Span* kspan = _SpanPool.New(); // k = n - page，我们要给CC的

			// 从nspan的头切一个下来
			kspan->_pageId = nspan->_pageId;
			kspan->_n = page;
			nspan->_pageId += page; // 往后退
			nspan->_n -= page;

			_PCSpanList[nspan->_n].PushFront(nspan);

			// 暂时不用的就，给头尾添加映射
			//_IdSpanMap[nspan->_pageId] = nspan;
			//_IdSpanMap[nspan->_pageId + nspan->_n - 1] = nspan; // 不减一就映射越界了

			_IdSpanMap.set(nspan->_pageId, nspan);
			_IdSpanMap.set(nspan->_pageId + nspan->_n - 1, nspan);


			// 用的就建立每块映射
			for (PAGE_ID i = 0; i < kspan->_n; i++)
			{
				//_IdSpanMap[kspan->_pageId + i] = kspan;
				_IdSpanMap.set(kspan->_pageId + i, kspan);
			}

			return kspan;
		}
	}

	// 没有更大的页了，得向堆申请空间

	Span* BigSpan = _SpanPool.New();
	void* ptr = SystemAlloc(MAXPAGES - 1); // 申请 128 个页  128 * 8k 的大块内存

	BigSpan->_pageId = (PAGE_ID)ptr >> PAGESHIFT; // 除8k就算页号，具体看博客
	BigSpan->_n = MAXPAGES - 1; // 大块内存的页倍数

	_PCSpanList[BigSpan->_n].PushFront(BigSpan); // 插入到PC的链表上

	return NewSpan(page); // 递归调用一下自己，再切分
}