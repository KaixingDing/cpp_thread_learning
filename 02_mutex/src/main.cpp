#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

class BankAccount
{
private:
    double balance;
    std::mutex mutex; // 保护balance的互斥量

public:
    BankAccount(double initial_balance) : balance(initial_balance) {}

    // 不使用互斥量的存款操作 - 可能导致数据竞争
    void deposit_unsafe(double amount)
    {
        double temp = balance;
        std::this_thread::sleep_for(std::chrono::microseconds(1)); // 模拟一些处理时间
        temp += amount;
        balance = temp;
    }

    // 使用互斥量的安全存款操作
    void deposit_safe(double amount)
    {
        std::lock_guard<std::mutex> lock(mutex); // RAII方式加锁
        double temp = balance;
        std::this_thread::sleep_for(std::chrono::microseconds(1)); // 模拟一些处理时间
        temp += amount;
        balance = temp;
    }

    double get_balance()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return balance;
    }
};

void test_race_condition(bool use_mutex)
{
    BankAccount account(0.0);
    std::vector<std::thread> threads;
    const int num_threads = 100;
    const double deposit_amount = 100.0;

    // 创建多个线程同时进行存款操作
    for (int i = 0; i < num_threads; ++i)
    {
        if (use_mutex)
        {
            threads.emplace_back([&account, deposit_amount]
                                 { account.deposit_safe(deposit_amount); });
        }
        else
        {
            threads.emplace_back([&account, deposit_amount]
                                 { account.deposit_unsafe(deposit_amount); });
        }
    }

    // 等待所有线程完成
    for (auto &thread : threads)
    {
        thread.join();
    }

    // 理论上的最终余额应该是 deposit_amount * num_threads
    double expected_balance = deposit_amount * num_threads;
    double actual_balance = account.get_balance();

    std::cout << "Expected balance: " << expected_balance << std::endl;
    std::cout << "Actual balance: " << actual_balance << std::endl;
    std::cout << "Difference: " << expected_balance - actual_balance << std::endl;
}

int main()
{
    std::cout << "Testing without mutex (unsafe):" << std::endl;
    test_race_condition(false);

    std::cout << "\nTesting with mutex (safe):" << std::endl;
    test_race_condition(true);

    return 0;
}