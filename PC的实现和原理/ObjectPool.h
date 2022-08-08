#pragma once
#include "Common.h"

template<class T>
class ObjectPool
{
public:
    T* New()
    {
        T* obj = nullptr;
        if (_freeList) //归还的内存可用
        {
            void* next = *((void**)_freeList); // 头删传递指针
            obj = (T*)_freeList;
            _freeList = next;
        }
        else {
            if (_mBytesSize < sizeof(T)) // 有可能不是整除，所以要和T比较大小
            {
                _mBytesSize = 128 * 1024;
                _memery = (char*)malloc(_mBytesSize);
                if (_memery == nullptr) {
                    throw std::bad_alloc();
                }
            }
            obj = (T*)_memery; // 预防指针大过剩余T类型,无法存下指针

            size_t Size = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
            _memery += Size; // 往后移
            _mBytesSize -= Size; // 用了多少减去多少
        }

        new(obj)T; // 定位new，显示构造函数初始化

        return obj;
    }

    void Delete(T* obj)
    {
        obj->~T(); // 在开头要显示调用析构函数清理对象
        //头插
        *(void**)obj = _freeList; // 为什么是void**，因为是指针大小，根据操作系统变化
        _freeList = obj;
    }

private:
    char* _memery = nullptr; // 开辟的空间
    void* _freeList = nullptr; // 返回空间的链表；
    size_t _mBytesSize = 0; // 空间中的剩余字节大小；
};


struct TreeNode
{
    int _val;
    TreeNode* _left;
    TreeNode* _right;

    TreeNode()
        :_val(0)
        , _left(nullptr)
        , _right(nullptr)
    {}
};

void TestObjectPool()
{
    // 申请释放的轮次
    const size_t Rounds = 5;

    // 每轮申请释放多少次
    const size_t N = 100000;

    std::vector<TreeNode*> v1;
    v1.reserve(N);

    size_t begin1 = clock();

    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v1.push_back(new TreeNode);
        }
        for (int i = 0; i < N; ++i)
        {
            delete v1[i];
        }
        v1.clear();
    }

    size_t end1 = clock();

    std::vector<TreeNode*> v2;
    v2.reserve(N);

    ObjectPool<TreeNode> TNPool;
    size_t begin2 = clock();
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v2.push_back(TNPool.New());
        }
        for (int i = 0; i < N; ++i)
        {
            TNPool.Delete(v2[i]);
        }
        v2.clear();
    }
    size_t end2 = clock();

    cout << "new cost time:" << end1 - begin1 << endl;
    cout << "object pool cost time:" << end2 - begin2 << endl;
}

