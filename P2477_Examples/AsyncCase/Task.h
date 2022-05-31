#ifndef TASK_H
#define TASK_H

#include "std-coroutine.h"
#include "SimpleExecutor.h"
#include <condition_variable>

class Task;
inline unsigned allocated_counts = 0;

struct Promise {
    struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }
        template <typename PromiseType>
        auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
            return h.promise().continuation;
        }
        void await_resume() noexcept {}
    };

    Task get_return_object() noexcept;
    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalAwaiter final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}

    template <class Awaitable>
    auto await_transform(Awaitable awaitable) noexcept requires requires {
        awaitable.coAwait((SimpleExecutor*)nullptr);
    }
    {
        return awaitable.coAwait(_executor);
    }

    void return_value(int v) {
        _value = v;
    }

    // Return the last argument if its type is std::coro_elision.
    // Return std::coro_elision::may otherwise.
    template <typename... Args>
    static constexpr std::coro_elision must_elide(Args&&... args) {
        using ArgsTuple = std::tuple<Args...>;
        constexpr std::size_t ArgsSize = std::tuple_size_v<ArgsTuple>;
        if constexpr (ArgsSize > 0) {
            using LastArgType = std::tuple_element_t<ArgsSize - 1, ArgsTuple>;
            if constexpr (std::is_convertible_v<LastArgType, std::coro_elision>)
                return std::get<ArgsSize - 1>(std::forward_as_tuple(args...));
        }

        return std::coro_elision::may;
    }

    void *operator new(std::size_t size) noexcept {
        allocated_counts++;
        return malloc(size);
    }

private:
    std::coroutine_handle<> continuation = std::noop_coroutine();
    SimpleExecutor *_executor = nullptr;
    int _value = 0;

    friend class Task;
};

class Task {
public:
    using promise_type = Promise;
    using HandleType = std::coroutine_handle<promise_type>;

    Task(HandleType handle) : handle(handle) {}
    Task(Task&& t) : handle(std::exchange(t.handle, nullptr)) {}
    ~Task() {
        if (handle)
            handle.destroy();
    }

    struct Awaiter {
        HandleType handle;
        Awaiter(HandleType handle) 
            : handle(HandleType::from_address(handle.address())) {}

        bool await_ready() const noexcept { return false; }

        template <typename U>
        void await_suspend(std::coroutine_handle<U> continuation) noexcept {
            handle.promise().continuation = continuation;
            std::function<void()> func = [h = handle]() mutable {
                h.resume();
            };
            assert(handle.promise()._executor);
            handle.promise()._executor->schedule(func);
        }

        int await_resume() noexcept {
            auto value =  handle.promise()._value;
            handle.destroy();
            return value;
        }
    };

    auto operator co_await() noexcept {
        return Task::Awaiter(std::exchange(handle, nullptr));
    }

    auto coAwait(SimpleExecutor *Executor) noexcept {
        handle.promise()._executor = Executor;
        return Task::Awaiter(std::exchange(handle, nullptr));
    }

    Task via(SimpleExecutor *Executor) noexcept {
        assert(handle);
        handle.promise()._executor = Executor;
        return Task(std::exchange(handle, nullptr));
    }

    int syncStart() {
        struct DetachedCoroutine {
            struct promise_type {
                std::suspend_never initial_suspend() noexcept { return {}; }
                std::suspend_never final_suspend() noexcept { return {}; }
                void return_void() noexcept {}
                void unhandled_exception() noexcept {}
                DetachedCoroutine get_return_object() noexcept {
                    return DetachedCoroutine();
                }

                std::coroutine_handle<> _continuation = nullptr;
            };
        };

        class Condition {
            public:
                void release() {
                    std::lock_guard lock(_mutex);
                    ++_count;
                    _condition.notify_one();
                }

                void acquire() {
                    std::unique_lock lock(_mutex);
                    _condition.wait(lock, [this] { return _count > 0; });
                    --_count;
                }

            private:
                std::mutex _mutex;
                std::condition_variable _condition;
                size_t _count = 0;
        };

        Condition Cond;
        int ret;
        auto launchCoro = [&Cond, &ret](Task task) -> DetachedCoroutine {
            ret = co_await task;
            Cond.release();
        };
        
        [[maybe_unused]] auto detached = launchCoro(std::move(*this));
        Cond.acquire();

        return ret;
    }

private:
    HandleType handle;
};

Task Promise::get_return_object() noexcept {
    return std::coroutine_handle<Promise>::from_promise(*this);
}

#endif
