#ifndef SIMPLE_EXECUTOR_H
#define SIMPLE_EXECUTOR_H

#include "ThreadPool.h"

class SimpleExecutor {
public:
    explicit SimpleExecutor(std::size_t threadNum) : _pool(threadNum) {}

    using Func = std::function<void()>;
    bool schedule(Func func) {
        return _pool.scheduleById(std::move(func));
    }

private:
    ThreadPool _pool;
};

#endif
