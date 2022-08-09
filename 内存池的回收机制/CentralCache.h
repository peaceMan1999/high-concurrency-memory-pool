#pragma once
#include "Common.h"

// 单例模式--饿汉模式，定义全局唯一实例化对象--中心缓存
class CentralCache
{
public:
	static CentralCache* GetInstance() // 获取实例，调用
	{
		return &_cc;
	}

	Span* GetOneSpan(SpanList& list, size_t bytes); // 假如Span为空，获取Span

	size_t GetRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes); // 从中心获取一定数量的对象给TC

	void ReleaseListToSpan(void* start, size_t bytes); // 处理从TC回收的自由链表

private:
	CentralCache()
	{};

	CentralCache(const CentralCache& p) = delete;

	static CentralCache _cc; // 对象

	SpanList _CCSpanList[MAXBUCKETS]; // CC的哈希表
};