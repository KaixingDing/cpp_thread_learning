#pragma once
#include <mutex>
#include <map>
#include <set>
#include <thread>

/**
 * @brief 资源分配图类，用于死锁检测
 *
 * 使用有向图表示线程和资源之间的关系：
 * - 记录每个线程持有的锁
 * - 记录每个线程等待的锁
 * - 通过环检测算法发现死锁
 */
class ResourceGraph
{
private:
    std::mutex graph_mutex;

    // 记录线程持有的锁：thread_id -> set<mutex*>
    std::map<std::thread::id, std::set<std::mutex *>> thread_holds;

    // 记录线程等待的锁：thread_id -> set<mutex*>
    std::map<std::thread::id, std::set<std::mutex *>> thread_waits;

    /**
     * @brief 使用深度优先搜索检测环
     * @param current 当前检查的线程ID
     * @param visited 已访问的线程集合
     * @param recursion_stack 当前递归栈中的线程集合
     * @return 是否存在环
     */
    bool hasCycle(std::thread::id current,
                  std::set<std::thread::id> &visited,
                  std::set<std::thread::id> &recursion_stack);

public:
    void acquireLock(std::mutex *mtx);
    void releaseLock(std::mutex *mtx);
    void waitForLock(std::mutex *mtx);
    void stopWaiting(std::mutex *mtx);
    bool hasDeadlock();
};