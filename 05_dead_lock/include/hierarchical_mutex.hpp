#pragma once
#include <mutex>
#include <stdexcept>
#include <climits>

/**
 * @brief 分层互斥锁
 *
 * 实现基于层级的锁定策略：
 * - 每个锁都有一个层级编号
 * - 只能按照从高层级到低层级的顺序获取锁
 * - 防止死锁的一种方法
 */
class HierarchicalMutex
{
private:
    std::mutex mutex;
    const unsigned long hierarchy_level;
    // 线程局部存储，记录当前线程的上一个锁的层级
    inline static thread_local unsigned long previous_hierarchy_level = ULONG_MAX;

public:
    explicit HierarchicalMutex(unsigned long level);
    void lock();
    void unlock();
    bool try_lock();
};