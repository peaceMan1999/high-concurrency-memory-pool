#define _CRT_SECURE_NO_WARNINGS 1
#include "ConcurrentAlloc.h"
//#include "ObjectPool.h"

//void Alloc1()
//{
//	for (int i = 0; i < 5; i++)
//	{
//		void* ptr = ConcurrentAlloc(7);
//	}
//}
//
//void Alloc2()
//{
//	for (int i = 0; i < 5; i++)
//	{
//		void* ptr = ConcurrentAlloc(6);
//	}
//}

void Alloc3()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 7; ++i)
	{
		void* ptr = ConcurrentAlloc(6);
		v.push_back(ptr);
	}

	for (auto e : v)
	{
		ConcurrentFree(e, 6);
	}
}

void Alloc4()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 7; ++i)
	{
		void* ptr = ConcurrentAlloc(8);
		v.push_back(ptr);
	}

	for (auto e : v)
	{
		ConcurrentFree(e, 8);
	}
}

void Test4()
{
	std::thread t1(Alloc3);
	t1.join(); // wait

	std::thread t2(Alloc4);
	t2.join();
}

//
//void Test()
//{
//	std::thread t1(Alloc1);
//	t1.join(); // wait
//
//	std::thread t2(Alloc2);
//	t2.join();
//}

//void Test2()
//{
//	void* p1 = ConcurrentAlloc(6);
//	void* p2 = ConcurrentAlloc(1);
//	void* p3 = ConcurrentAlloc(8);
//	void* p4 = ConcurrentAlloc(3);
//	void* p5 = ConcurrentAlloc(5);
//}

//void Test3()
//{
//	for (int i = 0; i < 1023; i++)
//	{
//		void* p1 = ConcurrentAlloc(6);
//		cout << p1 << endl;
//	}
//	
//	void* p1 = ConcurrentAlloc(8);
//	cout << p1 << endl;
//
//	void* p2 = ConcurrentAlloc(8);
//	cout << p2 << endl;
//
//}

int main()
{
	//Test();
	//Test2();
	//Test3();
	Test4();

	return 0;
}