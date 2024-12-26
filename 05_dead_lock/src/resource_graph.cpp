#include "resource_graph.hpp"

void ResourceGraph::acquireLock(std::mutex *mtx)
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    thread_holds[std::this_thread::get_id()].insert(mtx);
}

void ResourceGraph::releaseLock(std::mutex *mtx)
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    thread_holds[std::this_thread::get_id()].erase(mtx);
}

void ResourceGraph::waitForLock(std::mutex *mtx)
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    thread_waits[std::this_thread::get_id()].insert(mtx);
}

void ResourceGraph::stopWaiting(std::mutex *mtx)
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    thread_waits[std::this_thread::get_id()].erase(mtx);
}

bool ResourceGraph::hasDeadlock()
{
    std::lock_guard<std::mutex> lock(graph_mutex);
    std::set<std::thread::id> visited;
    std::set<std::thread::id> recursion_stack;

    for (const auto &[thread_id, _] : thread_holds)
    {
        if (visited.find(thread_id) == visited.end())
        {
            if (hasCycle(thread_id, visited, recursion_stack))
            {
                return true;
            }
        }
    }
    return false;
}

bool ResourceGraph::hasCycle(std::thread::id current,
                             std::set<std::thread::id> &visited,
                             std::set<std::thread::id> &recursion_stack)
{
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
                else if (recursion_stack.find(thread_id) != recursion_stack.end())
                {
                    return true;
                }
            }
        }
    }

    recursion_stack.erase(current);
    return false;
}