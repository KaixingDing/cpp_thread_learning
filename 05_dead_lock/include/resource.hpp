#pragma once
#include <mutex>

/**
 * @brief 模拟资源类
 *
 * 代表系统中的一个资源：
 * - 唯一标识符
 * - 访问控制互斥锁
 */
class Resource
{
private:
    int id;
    std::mutex mutex;

public:
    explicit Resource(int i);
    std::mutex &getMutex();
    int getId() const;
};