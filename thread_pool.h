#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <future>
#include <functional>
#include <iostream>
#include <queue>
#include <mutex>
#include <memory>
#include <time.h>

#ifdef __win32
#include <windows.h>
#else
#include <sys/time.h>
#endif
using namespace std;

void getNow(timeval *tv);
int64_t getNowMs();

#define TNOW getNow()
#define TNOWMS getNowMs()

class ThreadPool{
protected:
    struct TaskFunc{
        TaskFunc(int64_t expireTime):_expireTime(expireTime){}

        std::function<void()> _func;
        int64_t _expireTime = 0;
    };

    typedef shared_ptr<TaskFunc> TaskFuncPtr;


public:

    ThreadPool();
    ~ThreadPool();

    bool init(size_t num);

    size_t getThreadNum(){
        std::scoped_lock<std::mutex> lock(_mutex);
        return _threads.size();
    }

    size_t getJobNum(){
        std::scoped_lock<std::mutex> lock(_mutex);
        return _tasks.size();
    }

    void stop();

    bool start();
    template<class F, class... Args>

    auto exec(F && f, Args... args)->std::future<decltype(f(args...))>{
        return exec(0, std::forward<F>(f), std::forward<Args>(args)...);
    }
    template<class F, class... Args>
    auto exec(int64_t timeoutMs, F &&f, Args&&... args) -> std::future<decltype(f(args...))>{
        int64_t expireTime = (timeoutMs == 0 ? 0 : TNOWMS + timeoutMs);
        using RetType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        TaskFuncPtr fptr = std::make_shared<TaskFunc>(expireTime);
        fptr->_func = [task](){
            (*task)();
        };

        std::scoped_lock<std::mutex> lock(_mutex);
        _tasks.push(fptr);
        _condition.notify_one();
        return task->get_future();
    }

    bool waitForAllDone(int millsecond = -1);


protected:
    bool get(TaskFuncPtr& task);
    bool isTerminate(){
        return _bTerminate;
    }

    void run();

protected:
    queue<TaskFuncPtr> _tasks;
    std::vector<std::thread> _threads;
    std::mutex _mutex;
    std::condition_variable _condition;

    size_t _threadNum;
    bool _bTerminate;
    std::atomic<int> _atomicCnt{0};

};


#endif