cmake_minimum_required(VERSION 3.10)
project(thread_learning)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找线程库
find_package(Threads REQUIRED)

# 添加可执行文件
add_executable(thread_demo src/main.cpp)

# 链接线程库
target_link_libraries(thread_demo PRIVATE Threads::Threads)