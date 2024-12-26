#include "tracked_mutex.hpp"

TrackedMutex::TrackedMutex(std::mutex *m, ResourceGraph *g)
    : mtx(m), graph(g) {}

TrackedMutex::~TrackedMutex()
{
    if (locked)
    {
        unlock();
    }
}

bool TrackedMutex::try_lock()
{
    graph->waitForLock(mtx);
    if (mtx->try_lock())
    {
        locked = true;
        graph->stopWaiting(mtx);
        graph->acquireLock(mtx);
        return true;
    }
    graph->stopWaiting(mtx);
    return false;
}

void TrackedMutex::lock()
{
    graph->waitForLock(mtx);
    mtx->lock();
    locked = true;
    graph->stopWaiting(mtx);
    graph->acquireLock(mtx);
}

void TrackedMutex::unlock()
{
    if (locked)
    {
        graph->releaseLock(mtx);
        mtx->unlock();
        locked = false;
    }
}