#include <iostream>
#include <thread>
#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <atomic>
#include <exception>
#include <memory>

/**
 * @brief 增强版线程池实现
 *
 * 特性：
 * 1. 支持任务优先级
 * 2. 支持任务取消
 * 3. 支持任务超时
 * 4. 支持异常处理
 * 5. 支持优雅关闭
 * 6. 提供详细的运行时统计
 */
class ThreadPool
{
private:
    /**
     * @brief 任务包装器，支持优先级和取消功能
     */
    struct Task
    {
        std::function<void()> func;                     // 任务函数
        int priority;                                   // 优先级（数字越大优先级越高）
        std::chrono::steady_clock::time_point deadline; // 任务截止时间
        bool cancelled{false};                          // 取消标志

        // 构造函数，设置任务的优先级和超时时间
        Task(std::function<void()> f, int p = 0,
             std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
            : func(std::move(f)), priority(p), deadline(std::chrono::steady_clock::now() + timeout) {}

        // 优先级比较，用于优先队列排序
        bool operator<(const Task &other) const
        {
            return priority < other.priority;
        }
    };

    // 线程池状态和同步相关成员
    std::vector<std::thread> workers;  // 工作线程集合
    std::priority_queue<Task> tasks;   // 任务优先队列
    mutable std::mutex queue_mutex;    // 队列互斥锁
    std::condition_variable condition; // 条件变量
    std::atomic<bool> stop{false};     // 停止标志

    // 统计信息
    std::atomic<int> active_threads{0};       // 活跃线程计数
    std::atomic<uint64_t> completed_tasks{0}; // 已完成任务计数
    std::atomic<uint64_t> failed_tasks{0};    // 失败任务计数
    std::atomic<uint64_t> timeout_tasks{0};   // 超时任务计数
    std::atomic<uint64_t> cancelled_tasks{0}; // 取消任务计数

    /**
     * @brief 工作线程的主循环函数
     *
     * 工作线程不断从任务队列中获取任务并执行，直到线程池停止
     * 包含了任务超时检查、取消检查和异常处理
     */
    void worker_thread()
    {
        while (true)
        {
            std::function<void()> task;
            bool has_timeout = false;
            bool is_cancelled = false;

            // 获取任务
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                // 等待直到有任务或被通知停止
                condition.wait(lock, [this]
                               { return stop || !tasks.empty(); });

                // 如果线程池停止且没有待处理任务，则退出
                if (stop && tasks.empty())
                {
                    return;
                }

                // 检查任务是否超时或已取消
                if (!tasks.empty())
                {
                    auto now = std::chrono::steady_clock::now();
                    const Task &top_task = tasks.top();

                    if (top_task.deadline < now)
                    {
                        has_timeout = true;
                        timeout_tasks++;
                    }
                    else if (top_task.cancelled)
                    {
                        is_cancelled = true;
                        cancelled_tasks++;
                    }
                    else
                    {
                        task = std::move(top_task.func);
                    }
                    tasks.pop();
                }
            }

            // 如果任务有效（未超时且未取消），则执行任务
            if (task && !has_timeout && !is_cancelled)
            {
                active_threads++;

                try
                {
                    task();
                    completed_tasks++;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Task exception: " << e.what() << std::endl;
                    failed_tasks++;
                }
                catch (...)
                {
                    std::cerr << "Unknown task exception" << std::endl;
                    failed_tasks++;
                }

                active_threads--;
            }
        }
    }

public:
    /**
     * @brief 构造函数
     * @param threads 工作线程数量，默认为硬件支持的并发线程数
     */
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency())
        : stop(false)
    {
        for (size_t i = 0; i < threads; ++i)
        {
            workers.emplace_back([this]
                                 { worker_thread(); });
        }
    }

    /**
     * @brief 析构函数
     *
     * 确保所有任务完成后再关闭线程池
     */
    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }

        condition.notify_all();
        for (std::thread &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    /**
     * @brief 提交任务到线程池
     *
     * @param priority 任务优先级
     * @param timeout 任务超时时间
     * @param f 任务函数
     * @param args 任务函数参数
     * @return std::future<> 用于获取任务结果
     *
     * @throws std::runtime_error 如果线程池已停止
     */
    template <class F, class... Args>
    auto submit(int priority, std::chrono::milliseconds timeout, F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        // 创建任务包装器
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();

        // 将任务添加到队列
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop)
            {
                throw std::runtime_error("ThreadPool: submitting on a stopped pool");
            }

            tasks.emplace([task]()
                          { (*task)(); }, priority, timeout);
        }

        condition.notify_one();
        return res;
    }

    /**
     * @brief 获取待处理任务数量
     */
    size_t get_pending_tasks() const
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        return tasks.size();
    }

    /**
     * @brief 获取线程池统计信息
     */
    struct Statistics
    {
        int active_threads;
        uint64_t completed_tasks;
        uint64_t failed_tasks;
        uint64_t timeout_tasks;
        uint64_t cancelled_tasks;
        size_t pending_tasks;
    };

    Statistics get_statistics() const
    {
        return Statistics{
            active_threads,
            completed_tasks,
            failed_tasks,
            timeout_tasks,
            cancelled_tasks,
            get_pending_tasks()};
    }

    /**
     * @brief 等待所有任务完成
     * @param timeout 最大等待时间，默认无限等待
     * @return 是否在超时前完成所有任务
     */
    bool wait_all(std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
    {
        auto start = std::chrono::steady_clock::now();
        while (get_pending_tasks() > 0 || active_threads > 0)
        {
            if (std::chrono::steady_clock::now() - start > timeout)
            {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }
};

/**
 * @brief 测试用的计算任务
 * 使用 long long 避免整数溢出
 */
long long compute_task(int id, int complexity)
{
    std::cout << "Task " << id << " started in thread "
              << std::this_thread::get_id() << std::endl;

    long long result = 0;
    for (int i = 0; i < complexity * 1000000; ++i)
    {
        result += i;
    }

    std::cout << "Task " << id << " completed with result "
              << result << std::endl;
    return result;
}

/**
 * @brief 定期打印线程池状态的监控函数
 */
void print_pool_status(const ThreadPool::Statistics &stats)
{
    std::cout << "\nThread Pool Status:"
              << "\nPending tasks: " << stats.pending_tasks
              << "\nActive threads: " << stats.active_threads
              << "\nCompleted tasks: " << stats.completed_tasks
              << "\nFailed tasks: " << stats.failed_tasks
              << "\nTimeout tasks: " << stats.timeout_tasks
              << "\nCancelled tasks: " << stats.cancelled_tasks
              << std::endl;
}

int main()
{
    // 创建线程池
    ThreadPool pool(4);
    std::vector<std::future<long long>> results;

    // 提交任务
    std::cout << "Submitting tasks...\n";
    for (int i = 0; i < 8; ++i)
    {
        int priority = rand() % 10;
        int complexity = rand() % 5 + 1;

        // 设置任务超时时间为5秒
        auto timeout = std::chrono::milliseconds(5000);

        results.emplace_back(
            pool.submit(priority, timeout, compute_task, i, complexity));

        std::cout << "Submitted task " << i
                  << " with priority " << priority
                  << " and complexity " << complexity << std::endl;
    }

    // 监控线程池状态
    std::thread monitor([&pool]()
                        {
        while (pool.get_statistics().pending_tasks > 0 || 
               pool.get_statistics().active_threads > 0) {
            print_pool_status(pool.get_statistics());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } });

    // 获取任务结果
    std::cout << "\nGetting results...\n";
    for (size_t i = 0; i < results.size(); ++i)
    {
        try
        {
            std::cout << "Task " << i << " result: "
                      << results[i].get() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Task " << i << " failed: " << e.what() << std::endl;
        }
    }

    monitor.join();

    // 打印最终统计信息
    auto final_stats = pool.get_statistics();
    std::cout << "\nFinal Statistics:\n";
    print_pool_status(final_stats);

    return 0;
}