#pragma once
#include "Common.h"


// 也是一个单例模式，全局唯一
class PageCache
{
public:
	static PageCache* GetInstance() // 获取实例，调用
	{
		return &_PC;
	}

	Span* NewSpan(size_t page); // 创建对应的span

	std::mutex PC_mtx; // 整个PC一个锁，公共

private:
	PageCache()
	{};

	PageCache(const PageCache& p) = delete;

	static PageCache _PC;

	SpanList _PCSpanList[MAXPAGES];

};






