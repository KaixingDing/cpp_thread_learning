#include <iostream>
#include <cassert>
#include "resource_graph.hpp"
#include "tracked_mutex.hpp"
#include "hierarchical_mutex.hpp"

void test_resource_graph()
{
    ResourceGraph graph;
    std::mutex m1, m2;

    // 测试基本功能
    graph.waitForLock(&m1);
    assert(!graph.hasDeadlock());

    graph.acquireLock(&m1);
    assert(!graph.hasDeadlock());

    graph.releaseLock(&m1);
    assert(!graph.hasDeadlock());
}

void test_hierarchical_mutex()
{
    HierarchicalMutex high(2000);
    HierarchicalMutex low(1000);

    // 测试正确的锁定顺序
    high.lock();
    low.lock();
    low.unlock();
    high.unlock();

    // 测试错误的锁定顺序
    bool exception_caught = false;
    try
    {
        low.lock();
        high.lock(); // 应该抛出异常
    }
    catch (const std::runtime_error &)
    {
        exception_caught = true;
        low.unlock();
    }
    assert(exception_caught);
}

int main()
{
    test_resource_graph();
    test_hierarchical_mutex();

    std::cout << "All tests passed!\n";
    return 0;
}