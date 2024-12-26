# 添加测试可执行文件
add_executable(deadlock_test deadlock_test.cpp)
target_link_libraries(deadlock_test PRIVATE Threads::Threads)

# 添加测试
add_test(NAME DeadlockTest COMMAND deadlock_test)