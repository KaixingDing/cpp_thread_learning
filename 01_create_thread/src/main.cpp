#include <iostream>
#include <thread>
#include <chrono>

void print_hello()
{
    std::cout << "Hello from thread "
              << std::this_thread::get_id()
              << std::endl;

    // 模拟一些工作
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Thread "
              << std::this_thread::get_id()
              << " finished" << std::endl;
}

int main()
{
    std::cout << "Main thread id: "
              << std::this_thread::get_id()
              << std::endl;

    // 创建一个新线程
    std::thread t1(print_hello);

    // 等待线程完成, 主线程会阻塞在这里
    // t1.join();

    // 分离线程, 主线程不再等待子线程, 在这个程序里如果不睡眠的话大概率看不到子线程的输出
    t1.detach();

    // 睡眠一段时间, 等待子线程完成
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Main thread finished" << std::endl;
    return 0;
}