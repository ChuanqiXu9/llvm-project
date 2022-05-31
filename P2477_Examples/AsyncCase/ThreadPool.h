#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <functional>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    struct WorkItem {
        std::function<void()> fn = nullptr;
    };

    enum ERROR_TYPE {
        ERROR_NONE = 0,
        ERROR_POOL_HAS_STOP,
        ERROR_POOL_ITEM_IS_NULL,
    };

    explicit ThreadPool(size_t threadNum = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool::ERROR_TYPE scheduleById(std::function<void()> fn,
                                        int32_t id = -1);
    int32_t getCurrentId() const;
    size_t getItemCount() const;
    int32_t getThreadNum() const { return _threadNum; }

private:
    std::pair<size_t, ThreadPool *> *getCurrent() const;
    int32_t _threadNum;

    std::vector<std::queue<WorkItem>> _queues;
    std::vector<std::thread> _threads;

    std::atomic<bool> _stop;
};

inline ThreadPool::ThreadPool(size_t threadNum)
    : _threadNum(threadNum ? threadNum : std::thread::hardware_concurrency()),
      _queues(_threadNum),
      _stop(false) {
    auto worker = [this](size_t id) {
        auto current = getCurrent();
        current->first = id;
        current->second = this;
        while (!_stop) {
            if (_queues[id].empty())
                continue;

            WorkItem workerItem = _queues[id].front();
            _queues[id].pop();

            if (workerItem.fn)
                workerItem.fn();
        }
    };

    _threads.reserve(_threadNum);

    for (auto i = 0; i < _threadNum; ++i) {
        _threads.emplace_back(worker, i);
    }
}

inline ThreadPool::~ThreadPool() {
    _stop = true;
    for (auto &thread : _threads)
        thread.join();
}

inline ThreadPool::ERROR_TYPE ThreadPool::scheduleById(std::function<void()> fn,
                                                       int32_t id) {
    if (nullptr == fn) {
        return ERROR_POOL_ITEM_IS_NULL;
    }

    if (_stop) {
        return ERROR_POOL_HAS_STOP;
    }

    if (id == -1) {
        id = rand() % _threadNum;
        _queues[id].push(
            WorkItem{std::move(fn)});
    } else {
        assert(id < _threadNum);
        _queues[id].push(WorkItem{std::move(fn)});
    }

    return ERROR_NONE;
}

inline std::pair<size_t, ThreadPool *> *ThreadPool::getCurrent() const {
    static thread_local std::pair<size_t, ThreadPool *> current(-1, nullptr);
    return &current;
}

inline int32_t ThreadPool::getCurrentId() const {
    auto current = getCurrent();
    if (this == current->second) {
        return current->first;
    }
    return -1;
}

inline size_t ThreadPool::getItemCount() const {
    size_t ret = 0;
    for (auto i = 0; i < _threadNum; ++i) {
        ret += _queues[i].size();
    }
    return ret;
}

#endif // THREAD_POOL_H
