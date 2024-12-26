#pragma once
#include "resource_graph.hpp"
#include <mutex>

/**
 * @brief 可追踪的互斥锁包装器
 *
 * 包装标准互斥锁，添加资源获取和释放的追踪功能：
 * - 记录锁的获取尝试
 * - 记录锁的实际获取
 * - 记录锁的释放
 * - 自动清理（RAII）
 */
class TrackedMutex
{
private:
    std::mutex *mtx;      // 底层互斥锁
    ResourceGraph *graph; // 资源图引用
    bool locked = false;  // 锁定状态

public:
    TrackedMutex(std::mutex *m, ResourceGraph *g);
    ~TrackedMutex();

    bool try_lock();
    void lock();
    void unlock();
};