cmake_minimum_required(VERSION 3.10)
project(DeadlockDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

# 收集源文件
file(GLOB SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(deadlock_demo ${SOURCES})

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(deadlock_demo PRIVATE Threads::Threads)

# # 启用测试
# enable_testing()
# add_subdirectory(test)