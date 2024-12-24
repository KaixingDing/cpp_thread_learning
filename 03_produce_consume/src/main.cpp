#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <random>
#include <chrono>

template <typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> queue;
    mutable std::mutex mutex;
    std::condition_variable not_empty;
    std::condition_variable not_full;
    size_t capacity;

public:
    explicit ThreadSafeQueue(size_t max_capacity) : capacity(max_capacity) {}

    // 生产者接口
    void produce(T item)
    {
        std::unique_lock<std::mutex> lock(mutex);

        // 等待队列有空间
        not_full.wait(lock, [this]()
                      { return queue.size() < capacity; });

        queue.push(std::move(item));
        std::cout << "Produced: " << item
                  << " Queue size: " << queue.size() << std::endl;

        // 通知消费者
        not_empty.notify_one();
    }

    // 消费者接口
    T consume()
    {
        std::unique_lock<std::mutex> lock(mutex);

        // 等待队列非空
        not_empty.wait(lock, [this]()
                       { return !queue.empty(); });

        T item = std::move(queue.front());
        queue.pop();
        std::cout << "Consumed: " << item
                  << " Queue size: " << queue.size() << std::endl;

        // 通知生产者
        not_full.notify_one();

        return item;
    }

    // 当前队列大小
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }
};

// 生产者函数
void producer(ThreadSafeQueue<int> &queue, int id, int items_to_produce)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay(100, 500);

    for (int i = 0; i < items_to_produce; ++i)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(delay(gen)));
        queue.produce(id * 1000 + i);
    }
}

// 消费者函数
void consumer(ThreadSafeQueue<int> &queue, int items_to_consume)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay(200, 800);

    for (int i = 0; i < items_to_consume; ++i)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(delay(gen)));
        int item = queue.consume();
    }
}

int main()
{
    ThreadSafeQueue<int> queue(2); // 容量为5的队列

    const int num_producers = 2;
    const int num_consumers = 3;
    const int items_per_producer = 9;
    const int items_per_consumer =
        (num_producers * items_per_producer) / num_consumers;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // 启动生产者线程
    for (int i = 0; i < num_producers; ++i)
    {
        producers.emplace_back(producer,
                               std::ref(queue), i + 1, items_per_producer);
    }

    // 启动消费者线程
    for (int i = 0; i < num_consumers; ++i)
    {
        consumers.emplace_back(consumer,
                               std::ref(queue), items_per_consumer);
    }

    // 等待所有线程完成
    for (auto &p : producers)
        p.join();
    for (auto &c : consumers)
        c.join();

    return 0;
}