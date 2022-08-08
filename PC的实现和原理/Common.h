#pragma once
#include <iostream>
#include <vector>
#include <time.h>
#include <assert.h>
#include <thread>
#include <algorithm>
#include <mutex>

using std::cout;
using std::endl;

// 尽量不用宏
static const size_t MAXBYTES = 258 * 1024;  // 最大bytes
static const size_t MAXBUCKETS = 208;		// 最大桶数
static const size_t MAXPAGES = 129;			// 最大页数
static const size_t PAGESHIFT = 13;			// 除或乘以8k



// 64要放前面，因为x64下既有32又有64
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else
	// linux
#endif

#ifdef _WIN64
	#include <windows.h>	
#elif _WIN32
	#include <windows.h>	
#else
	// linux
#endif

// 直接去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage) // 页
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux下brk mmap等
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

// 定义静态函数
static void*& NextObj(void* obj)
{
	return *(void**)obj; // 前4/8位
}

// 定义一个类，是用来管理切分好的对象的自由链表
class FreeList
{
public:
	void Push(void* obj) // 回收头插
	{
		assert(obj);

		NextObj(obj) = _freeList; // 定长内存池讲过
		_freeList = obj;
	}

	void PushRange(void* start, void* end)
	{
		NextObj(end) = _freeList;
		_freeList = start;
	}

	void* Pop() // 发放内存，头删
	{
		assert(_freeList);

		void* obj = _freeList;
		_freeList = NextObj(obj);
		return obj;
	}

	size_t& MaxSize()
	{
		return maxSize;
	}

	bool Empty() // 判空
	{
		return _freeList == nullptr;
	}
private:
	void* _freeList = nullptr; // 给一个指针作为头节点
	size_t maxSize = 1;
};


// 定义一个类，用来计算对齐字节和桶
class SizeClass
{
public:
	// 为什么用静态内联？因为成员函数需要用一个类去调用，且SizeClass类没有成员，又经常调用，为了方便
	static inline size_t _RoundUp(size_t bytes, size_t alignByte)
	{
		return ((bytes + alignByte - 1) & ~(alignByte - 1)); // 例如 8 + 7 = 15 &~ 7 = 8;具体看博客；
	}

	static inline size_t RoundUp(size_t bytes) // 计算实际对齐后的比特位大小
	{
		// 整体控制在最多10%左右的内碎片浪费，具体原理看我的博客；
		// [1,128]					8byte对齐			freelist[0,16)
		// [128+1,1024]				16byte对齐			freelist[16,72)
		// [1024+1,8*1024]			128byte对齐			freelist[72,128)
		// [8*1024+1,64*1024]		1024byte对齐		freelist[128,184)
		// [64*1024+1,256*1024]		8*1024byte对齐		freelist[184,208)
		assert(bytes <= MAXBYTES); // 真不执行，假执行
		if (bytes <= 128) { // 小于128就是8字节对齐
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024) {
			return _RoundUp(bytes, 16);
		}
		else if (bytes <= 8 * 1024) {
			return _RoundUp(bytes, 128);
		}
		else if (bytes <= 64 * 1024) {
			return _RoundUp(bytes, 1024);
		}
		else if (bytes <= 256 * 1024) {
			return _RoundUp(bytes, 8 * 1024);
		}
		else { // 判断失败，有问题
			cout << "RoundUp err" << endl;
			return -1;
		}
	}

	static inline size_t _Index(size_t bytes, size_t alignShift)
	{
		return (((bytes + (1 << alignShift) -1) >> alignShift)-1);
		// 例如  1 << 3 = 8; 1~8 + 7 = 8~15; 8 >> 3 = 1、15 >> 3 = 1; -1 = 0; 代表1~8在0号桶
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAXBYTES);
		// 在上面我们可以看到每个阶段桶的位置和数量最多不超过208个
		static size_t bucketArray[4] = { 16, 72, 128, 184 };
		if (bytes <= 128) { // 128范围对应的是0~15号桶，一共16个
			return _Index(bytes, 3); // alignShift代表1的左移次数/2的次方
		}
		else if (bytes <= 1024) {
			return _Index(bytes, 4) + bucketArray[0]; // 要加上前面的桶，具体看博客
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes, 7) + bucketArray[1];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes, 10) + bucketArray[2];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes, 13) + bucketArray[3];
		}
		else { // 判断失败，有问题
			cout << "Index err" << endl;
			return -2;
		}
	}

	static size_t NumMoveSize(size_t bytes) // 一次拿去多少对象
	{
		assert(bytes <= MAXBYTES);
		assert(bytes > 0);
		// [2, 512] 是最好的范围，具体看博客
		size_t num = MAXBYTES / bytes;

		if (num < 2) // 最小调节
		{
			num = 2;
		}
		if (num > 512) // 最大调节
		{
			num = 512;
		}

		return num;
	}

	// 计算一次向系统获取几个页
	// 单个对象 8byte
	// ...
	// 单个对象 256KB
	static size_t NumMovePage(size_t bytes)
	{
		size_t num = NumMoveSize(bytes);
		size_t npage = num * bytes;

		npage >>= PAGESHIFT;
		if (npage == 0) // 一个都不到就给一个
			npage = 1;

		return npage;
	}
};

struct Span
{
	PAGE_ID _pageId = 0; // 页号
	size_t _n = 0; // 页数量，页的倍数，就是PC中的每一个下标
	Span* _next = nullptr; // 双链表
	Span* _prev = nullptr;
	size_t _usecount = 0; // 属于归还内存
	void* _SpanFreeList = nullptr; // 切好的自由链表
};

class SpanList // 用来管理每一个bytes代表的Span链表
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End() // 因为是双向链表，所以end是头节点
	{
		return _head;
	}

	void PushFront(Span* obj)
	{
		Insert(Begin(), obj);
	}

	Span* PopFront() // 把span弹出去，尾删
	{
		Span* obj = _head->_next;
		Erase(obj);
		return obj;
	}

	bool Empty()
	{
		return _head == _head->_next;
	}

	void Insert(Span* pos, Span* obj) // 头插
	{
		assert(pos);
		assert(obj);
		Span* prev = pos->_prev;
		prev->_next = obj;
		obj->_prev = prev;

		obj->_next = pos;
		pos->_prev = obj;
	}

	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head); // 别把头节点删了
		pos->_prev->_next = pos->_next;
		pos->_next->_prev = pos->_prev;
	}

private:
	Span* _head; // 头节点
public:
	std::mutex CC_mtx; // 桶锁
};
