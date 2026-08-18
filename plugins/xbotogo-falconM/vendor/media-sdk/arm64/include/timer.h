#pragma once
// #include <iostream>
#include <functional>
#include <thread>
// #include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdint>

namespace blink {
namespace utils {

class Timer {
public:
    Timer();
    ~Timer();

    // 设置定时器在指定时间后执行回调函数
    void setTimeout(int64_t interval, std::function<void()> callback);

    // 设置定时器每隔一段时间执行回调函数
    void setInterval(int64_t interval, std::function<void()> callback);

    // 停止定时器
    void stop();

    bool isRunning();

private:
    std::atomic<bool> m_running;
    std::thread m_thread;
    std::mutex m_mtx;               // 互斥锁
    std::condition_variable m_cv;   // 条件变量，用于唤醒线程
};

}
}
