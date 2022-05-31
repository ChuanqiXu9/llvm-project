#include <type_traits>
#include <cstdlib>
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
    static constexpr std::coro_elision must_elide() {
        return std::coro_elision::always;
    }
};

struct TaskPromiseNeverElide : public TaskPromiseBase {
    static constexpr std::coro_elision must_elide() {
        return std::coro_elision::never;
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

struct TaskPromiseAlternative : public TaskPromiseBase {
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
};

using ControllableTask = Task<TaskPromiseAlternative>;

struct AlwaysElide {};
struct NeverElide {};
struct MayElide {};

template<typename T>
concept ElisionTag = (std::is_same_v<T, AlwaysElide> ||
                      std::is_same_v<T, NeverElide> ||
                      std::is_same_v<T, MayElide>);

template<ElisionTag Tag>
consteval std::coro_elision getStdCoroElision() {
  if constexpr (std::is_same_v<Tag, AlwaysElide>)
    return std::coro_elision::always;
  else if constexpr (std::is_same_v<Tag, NeverElide>)
    return std::coro_elision::never;
  else
    return std::coro_elision::may;
}

template <ElisionTag Tag>
struct TaskPromiseAlternativeEnhancement : public TaskPromiseBase {
    template <typename... Args>
    static constexpr std::coro_elision must_elide(Args&&... args) {
        return getStdCoroElision<Tag>();
    }
};

template <ElisionTag Tag>
using TemplateControllableTask = Task<TaskPromiseAlternativeEnhancement<Tag>>;

template <ElisionTag Tag>
struct MixedStylePromise : public TaskPromiseAlternative {
    template <typename... Args>
    static constexpr std::coro_elision must_elide(Args&&... args) {
        constexpr std::coro_elision elision_state = getStdCoroElision<Tag>();
        if constexpr (elision_state == std::coro_elision::always ||
                      elision_state == std::coro_elision::never)
            return elision_state;
        else
            return TaskPromiseAlternative::must_elide(std::forward<Args>(args)...);
    }
};

template <ElisionTag Tag>
using MixedStyleControllableTask = Task<MixedStylePromise<Tag>>;
