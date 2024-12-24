#include <iostream>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>

template <typename K, typename V>
class ThreadSafeCache
{
private:
    mutable std::shared_mutex mutex;
    mutable std::unordered_map<K, V> cache;

    V loadFromDB(const K &key) const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return V{};
    }

public:
    void write(const K &key, const V &value)
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cache[key] = value;
    }

    V read(const K &key) const
    {
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            auto it = cache.find(key);
            if (it != cache.end())
            {
                return it->second;
            }
        }

        V value = loadFromDB(key);
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            cache.insert_or_assign(key, value);
        }
        return value;
    }

    size_t size() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return cache.size();
    }

    void clear()
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        cache.clear();
    }
};

class Statistics
{
private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, int> stats;

public:
    void increment(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++stats[key];
    }

    void display() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto &[key, value] : stats)
        {
            std::cout << key << ": " << value << std::endl;
        }
    }
};

std::string get_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_time_t);

    std::stringstream ss;
    ss << std::put_time(now_tm, "%H:%M:%S");
    return ss.str();
}

void reader(ThreadSafeCache<int, std::string> &cache,
            int id, int iterations, Statistics &stats)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < iterations; ++i)
    {
        int key = dis(gen);
        auto value = cache.read(key);

        std::cout << get_timestamp() << " Reader " << id
                  << " read key " << key << std::endl;

        stats.increment("reads");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void writer(ThreadSafeCache<int, std::string> &cache,
            int id, int iterations, Statistics &stats)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < iterations; ++i)
    {
        int key = dis(gen);
        std::string value = "Value-" + std::to_string(i) + "-from-Writer-" + std::to_string(id);
        cache.write(key, value);

        std::cout << get_timestamp() << " Writer " << id
                  << " wrote key " << key << std::endl;

        stats.increment("writes");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    ThreadSafeCache<int, std::string> cache;
    Statistics stats;

    const int num_readers = 5;
    const int num_writers = 2;
    const int reader_iterations = 10;
    const int writer_iterations = 5;

    std::vector<std::thread> readers;
    std::vector<std::thread> writers;

    for (int i = 0; i < num_readers; ++i)
    {
        readers.emplace_back(reader, std::ref(cache), i + 1,
                             reader_iterations, std::ref(stats));
    }

    for (int i = 0; i < num_writers; ++i)
    {
        writers.emplace_back(writer, std::ref(cache), i + 1,
                             writer_iterations, std::ref(stats));
    }

    for (auto &r : readers)
        r.join();
    for (auto &w : writers)
        w.join();

    std::cout << "\nFinal Statistics:\n";
    stats.display();
    std::cout << "Final cache size: " << cache.size() << std::endl;

    return 0;
}