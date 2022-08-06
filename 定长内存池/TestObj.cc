#define _CRT_SECURE_NO_WARNINGS 1
#include "ConcurrentAlloc.h"
//#include "ObjectPool.h"

void Alloc1()
{
	for (int i = 0; i < 5; i++)
	{
		void* ptr = ConcurrentAlloc(7);
	}
}

void Alloc2()
{
	for (int i = 0; i < 5; i++)
	{
		void* ptr = ConcurrentAlloc(6);
	}
}

void Test()
{
	std::thread t1(Alloc1);
	t1.join(); // wait

	std::thread t2(Alloc2);
	t2.join();
}

int main()
{
	Test();

	return 0;
}