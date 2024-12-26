#include "resource_graph.hpp"

/**
 * @brief 记录线程获取到的锁
 * @param mtx 被线程获取的互斥锁指针
 */
void ResourceGraph::acquireLock(std::mutex *mtx)
{
    // 保护资源图的互斥锁
    std::lock_guard<std::mutex> lock(graph_mutex);
    // 记录当前线程持有的锁
    thread_holds[std::this_thread::get_id()].insert(mtx);
}

/**
 * @brief 记录线程释放的锁
 * @param mtx 被释放的互斥锁指针
 */
void ResourceGraph::releaseLock(std::mutex *mtx)
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    // 从当前线程持有的锁集合中移除
    thread_holds[std::this_thread::get_id()].erase(mtx);

    // 如果线程不再持有任何锁，清理该记录
    if (thread_holds[std::this_thread::get_id()].empty())
    {
        thread_holds.erase(std::this_thread::get_id());
    }
}

/**
 * @brief 记录线程正在等待的锁
 * @param mtx 正在等待的互斥锁指针
 */
void ResourceGraph::waitForLock(std::mutex *mtx)
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    // 记录当前线程正在等待的锁
    thread_waits[std::this_thread::get_id()].insert(mtx);
}

/**
 * @brief 记录线程停止等待某个锁（可能是获取到了或者放弃等待）
 * @param mtx 停止等待的互斥锁指针
 */
void ResourceGraph::stopWaiting(std::mutex *mtx)
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    // 从当前线程等待的锁集合中移除
    thread_waits[std::this_thread::get_id()].erase(mtx);

    // 如果线程不再等待任何锁，清理该记录
    if (thread_waits[std::this_thread::get_id()].empty())
    {
        thread_waits.erase(std::this_thread::get_id());
    }
}

/**
 * @brief 检查系统中是否存在死锁
 * @return 如果检测到死锁返回true，否则返回false
 */
bool ResourceGraph::hasDeadlock()
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    std::set<std::thread::id> visited;         // 记录已访问的线程
    std::set<std::thread::id> recursion_stack; // 当前递归栈中的线程

    // 对每个持有锁的线程进行深度优先搜索
    for (const auto &[thread_id, _] : thread_holds)
    {
        // 如果这个线程还未被访问过
        if (visited.find(thread_id) == visited.end())
        {
            // 从这个线程开始进行深度优先搜索，检查是否存在环
            if (hasCycle(thread_id, visited, recursion_stack))
            {
                return true; // 发现环，表示存在死锁
            }
        }
    }
    return false; // 没有发现环，不存在死锁
}

/**
 * @brief 使用深度优先搜索检测等待图中是否存在环
 * @param current 当前正在检查的线程ID
 * @param visited 记录所有已访问的线程
 * @param recursion_stack 当前递归栈中的线程
 * @return 如果检测到环返回true，否则返回false
 */
bool ResourceGraph::hasCycle(std::thread::id current,
                             std::set<std::thread::id> &visited,
                             std::set<std::thread::id> &recursion_stack)
{
<<<<<<< HEAD
    visited.insert(current);
    recursion_stack.insert(current);

    const auto &waiting_locks = thread_waits[current];

    for (const auto &lock : waiting_locks)
    {
        for (const auto &[thread_id, held_locks] : thread_holds)
        {
            if (held_locks.find(lock) != held_locks.end())
            {
                if (visited.find(thread_id) == visited.end())
                {
                    if (hasCycle(thread_id, visited, recursion_stack))
                    {
                        return true;
                    }
                }
=======
    visited.insert(current);         // 标记当前线程为已访问
    recursion_stack.insert(current); // 将当前线程加入递归栈

    // 获取当前线程正在等待的所有锁
    const auto &waiting_locks = thread_waits[current];

    // 对于当前线程等待的每个锁
    for (const auto &lock : waiting_locks)
    {
        // 查找持有这个锁的所有线程
        for (const auto &[thread_id, held_locks] : thread_holds)
        {
            // 如果找到了持有这个锁的线程
            if (held_locks.find(lock) != held_locks.end())
            {
                // 如果这个线程还未被访问过
                if (visited.find(thread_id) == visited.end())
                {
                    // 继续深度优先搜索
                    if (hasCycle(thread_id, visited, recursion_stack))
                    {
                        return true; // 发现环
                    }
                }
                // 如果这个线程已经在递归栈中，说明形成了环
>>>>>>> 1058755 (死锁检测和预防)
                else if (recursion_stack.find(thread_id) != recursion_stack.end())
                {
                    return true;
                }
            }
        }
    }

<<<<<<< HEAD
=======
    // 回溯时将当前线程从递归栈中移除
>>>>>>> 1058755 (死锁检测和预防)
    recursion_stack.erase(current);
    return false;
}