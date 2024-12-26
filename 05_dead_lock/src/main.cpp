#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "resource_graph.hpp"
#include "tracked_mutex.hpp"
#include "hierarchical_mutex.hpp"
#include "resource.hpp"

// 模拟可能导致死锁的场景
void simulateDeadlockScenario(ResourceGraph &graph)
{
    Resource res1(1), res2(2);
    std::vector<std::thread> threads;

    // 线程1：尝试按1->2的顺序获取资源
    threads.emplace_back([&]()
                         {
        TrackedMutex lock1(&res1.getMutex(), &graph);
        TrackedMutex lock2(&res2.getMutex(), &graph);

        lock1.lock();
        std::cout << "Thread 1 acquired resource " << res1.getId() << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        lock2.lock();
        std::cout << "Thread 1 acquired resource " << res2.getId() << std::endl;

        // 使用资源
        std::cout << "Thread 1 using resources " << res1.getId() 
                  << " and " << res2.getId() << std::endl;

        lock2.unlock();
        lock1.unlock(); });

    // 线程2：尝试按2->1的顺序获取资源
    threads.emplace_back([&]()
                         {
        TrackedMutex lock2(&res2.getMutex(), &graph);
        TrackedMutex lock1(&res1.getMutex(), &graph);

        lock2.lock();
        std::cout << "Thread 2 acquired resource " << res2.getId() << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        lock1.lock();
        std::cout << "Thread 2 acquired resource " << res1.getId() << std::endl;

        // 使用资源
        std::cout << "Thread 2 using resources " << res2.getId() 
                  << " and " << res1.getId() << std::endl;

        lock1.unlock();
        lock2.unlock(); });

    // 检测线程：定期检查是否存在死锁
    std::thread detector([&]()
                         {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (graph.hasDeadlock()) {
                std::cout << "Deadlock detected!" << std::endl;
                break;
            }
        } });

    for (auto &t : threads)
    {
        t.join();
    }
    detector.join();
}

// 演示死锁预防技术
void demonstrateDeadlockPrevention()
{
    std::cout << "\nDemonstrating deadlock prevention techniques:\n";

    // 1. 使用std::lock同时锁定多个互斥量
    {
        std::cout << "1. Using std::lock:\n";
        std::mutex m1, m2;
        std::lock(m1, m2); // 原子方式锁定多个互斥量
        std::lock_guard<std::mutex> lock1(m1, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(m2, std::adopt_lock);
        std::cout << "   Locked multiple mutexes atomically\n";
    }

    // 2. 使用std::scoped_lock（C++17）
    {
        std::cout << "2. Using std::scoped_lock:\n";
        std::mutex m1, m2;
        std::scoped_lock lock(m1, m2);
        std::cout << "   Locked multiple mutexes using scoped_lock\n";
    }

    // 3. 使用分层锁
    {
        std::cout << "3. Using hierarchical mutexes:\n";
        HierarchicalMutex m1(2000); // 高层级锁
        HierarchicalMutex m2(1000); // 低层级锁

        try
        {
            m1.lock(); // 正确：先锁定高层级
            std::cout << "   Acquired high-level lock\n";
            m2.lock(); // 正确：再锁定低层级
            std::cout << "   Acquired low-level lock\n";
            m2.unlock();
            m1.unlock();
        }
        catch (const std::runtime_error &e)
        {
            std::cout << "   Error: " << e.what() << std::endl;
        }

        try
        {
            m2.lock(); // 正确：锁定低层级
            m1.lock(); // 错误：违反层级约束
            m1.unlock();
            m2.unlock();
        }
        catch (const std::runtime_error &e)
        {
            std::cout << "   Expected error: " << e.what() << std::endl;
            m2.unlock();
        }
    }
}

int main()
{
    ResourceGraph graph;

    // std::cout << "Simulating potential deadlock scenario...\n";
    // simulateDeadlockScenario(graph);

    demonstrateDeadlockPrevention();

    return 0;
}