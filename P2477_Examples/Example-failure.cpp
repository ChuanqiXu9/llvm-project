// clang++ -std=c++20 -O3
#include "std-coroutine.h"

class TaskBase;

inline unsigned allocated_counts = 0;

struct TaskPromiseBase {
    struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }
        template <typename PromiseType>
        auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
            return h.promise().continuation;
        }
        void await_resume() noexcept {}
    };
    TaskBase get_return_object() noexcept;
    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalAwaiter final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}

    void return_value(int x) noexcept {
        _value = x;
    }

    void *operator new(std::size_t size) noexcept {
        allocated_counts++;
        return malloc(size);
    }

    std::coroutine_handle<> continuation = std::noop_coroutine();
    int _value;
};

struct TaskPromiseAlwaysElide : public TaskPromiseBase {
    // failure:
    //   'must_elide' function must return 'std::coro_elision'
    static constexpr bool must_elide() {
        return true;
    }
};

struct TaskPromiseNeverElide : public TaskPromiseBase {
    // failure:
    //   'must_elide' function must return 'std::coro_elision'
    static constexpr bool must_elide() {
        return false;
    }
};


class TaskBase {
public:
    TaskBase(std::coroutine_handle<> handle) : handle(handle) {}
    TaskBase(TaskBase&& t) : handle(std::exchange(t.handle, nullptr)) {}
    ~TaskBase() {
        if (handle)
            handle.destroy();
    }

protected:
    std::coroutine_handle<> handle;
};

TaskBase TaskPromiseBase::get_return_object() noexcept {
    return TaskBase(std::coroutine_handle<TaskPromiseBase>::from_promise(*this));
}

template<class PromiseType>
class Task : public TaskBase{
public:
    using promise_type = PromiseType;
    using HandleType = std::coroutine_handle<promise_type>;
    Task(TaskBase&& Base) : TaskBase(std::move(Base)) {}

    struct Awaiter {
        HandleType handle;
        Awaiter(std::coroutine_handle<> handle) : handle(HandleType::from_address(handle.address())) {}
        bool await_ready() const noexcept { return false; }
        template <typename U>
        auto await_suspend(std::coroutine_handle<U> continuation) noexcept {
            handle.promise().continuation = continuation;
            return handle;
        }
        int await_resume() noexcept {
            auto value =  handle.promise()._value;
            handle.destroy();
            return value;
        }
    };

    auto operator co_await() {
        return Task::Awaiter(std::exchange(handle, nullptr));
    }

    int start() {
        handle.resume();
        assert(handle.done());
        auto &Promise = HandleType::from_address(handle.address()).promise();
        auto res = Promise._value;
        handle.destroy();
        handle = nullptr;
        return res;
    }
};

using NormalTask = Task<TaskPromiseBase>;
using AlwaysElideTask = Task<TaskPromiseAlwaysElide>;
using NeverElideTask = Task<TaskPromiseNeverElide>;

AlwaysElideTask foo() {
    co_return 43;
}

NeverElideTask foo() {
    co_return 43;
}
