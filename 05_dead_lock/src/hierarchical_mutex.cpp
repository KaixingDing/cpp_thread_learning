#include "hierarchical_mutex.hpp"

HierarchicalMutex::HierarchicalMutex(unsigned long level)
    : hierarchy_level(level) {}

void HierarchicalMutex::lock()
{
    // 检查是否违反层级约束
    // 当前锁的层级必须大于之前锁的层级
    if (previous_hierarchy_level <= hierarchy_level)
    {
        throw std::runtime_error("Mutex hierarchy violated");
    }
    mutex.lock();
    // 更新前一个层级
    previous_hierarchy_level = hierarchy_level;
}

void HierarchicalMutex::unlock()
{
    // 恢复前一个层级
    previous_hierarchy_level = hierarchy_level;
    mutex.unlock();
}

bool HierarchicalMutex::try_lock()
{
    if (previous_hierarchy_level <= hierarchy_level)
    {
        return false;
    }
    if (mutex.try_lock())
    {
        previous_hierarchy_level = hierarchy_level;
        return true;
    }
    return false;
}