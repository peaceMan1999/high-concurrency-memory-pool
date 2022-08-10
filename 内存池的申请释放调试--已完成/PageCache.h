#pragma once
#include "Common.h"
#include "ObjectPool.h"


// 也是一个单例模式，全局唯一
class PageCache
{
public:
	static PageCache* GetInstance() // 获取实例，调用
	{
		return &_PC;
	}

	Span* NewSpan(size_t page); // 创建对应的span

	Span* MapObjectToSpan(void* obj); // 获取从对象到span的对应关系

	void ReleaseSpanToPC(Span* span); // 合并内存

	std::mutex PC_mtx; // 整个PC一个锁，公共

private:
	PageCache()
	{};

	ObjectPool<Span> _SpanPool;

	PageCache(const PageCache& p) = delete;

	static PageCache _PC;
	
	std::unordered_map<PAGE_ID, Span*> _IdSpanMap; // 映射关系

	SpanList _PCSpanList[MAXPAGES];

};






