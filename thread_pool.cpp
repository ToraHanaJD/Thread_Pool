#include "thread_pool.h"

ThreadPool::ThreadPool():_threadNum(1), _bTerminate(false){}

ThreadPool::~ThreadPool(){ stop(); }

bool ThreadPool::init(size_t num){
    std::scoped_lock<std::mutex> lock(_mutex);
    if(!_threads.empty()) return false;
    _threadNum = num;
    return true;
}

void ThreadPool::stop(){
    {
        std::scoped_lock<std::mutex> lock(_mutex);
        _bTerminate = true;
    }

    _condition.notify_all();

    for(auto & t : _threads){
        if(t.joinable()) t.join();
    }
    {
        std::scoped_lock<std::mutex> lock(_mutex);
        _threads.clear();
    }
}

bool ThreadPool::start(){
    {
        std::scoped_lock<std::mutex> lock(_mutex);
        if(!_threads.empty()) return false;
        for(size_t i = 0; i < _threadNum; i++){
            _threads.emplace_back(&ThreadPool::run, this);
        }
    }
    return true;
}

bool ThreadPool::get(TaskFuncPtr & task){
    std::unique_lock<std::mutex> lock(_mutex);
    // 检查谓词条件，如果条件满足，则立即返回。
    // 如果条件不满足，则解锁互斥量，进入等待状态。
    _condition.wait(lock, [this](){return !_tasks.empty() || _bTerminate;});
    if(_bTerminate) return false;
    if(!_tasks.empty()){
        task = std::move(_tasks.front());
        _tasks.pop();
        return true;
    }
    return false;
}

void ThreadPool::run(){
    while(!isTerminate()){
        TaskFuncPtr task;
        bool ok = get(task);
        if(ok){
            ++_atomicCnt;
            try{
                if(task->_expireTime != 0 && task->_expireTime < TNOWMS){
                    task->_func();
                }else{
                    task->_func();
                }
            }catch(...){

            }
            --_atomicCnt;
            {
                std::scoped_lock<std::mutex> lock(_mutex);
                if(_atomicCnt == 0 && _tasks.empty()) _condition.notify_all();
            }
        }
    }
}

bool ThreadPool::waitForAllDone(int millsecond){
    std::unique_lock<std::mutex> lock(_mutex);
    if(_tasks.empty()) return true;
    if(millsecond < 0){
        _condition.wait(lock, [this](){
            return _tasks.empty() && _atomicCnt == 0;
        });
    }else{
        return _condition.wait_for(lock, std::chrono::milliseconds(millsecond), [this](){
            return _tasks.empty() && _atomicCnt == 0;
        });
    }
    return false;
}

#ifdef __WIN32
int gettimeofday(struct timeval & tv){
    time_t clock;
    struct tm tm;
    SYSTEMTIMME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_month = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinnute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tv.tv_sec = clock;
    tv.tv_usec = wtm.wMilliseconds * 1000;

    return 0;
}
#endif


void getNow(timeval * ty){
#ifdef __win32
    gettimeofday(*ty);
#else
    gettimeofday(ty, NULL);
#endif
}

int64_t getNowMs(){
    struct timeval tv;
    getNow(&tv);
    return tv.tv_sec * (int64_t)1000 + tv.tv_usec / 1000;
}