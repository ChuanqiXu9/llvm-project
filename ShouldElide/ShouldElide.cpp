// clang++ -std=c++2a -O3 ShouldElide.cpp
#include <type_traits>
#include <tuple>
#include <iostream>
#include "experimental/coroutine"

class TaskBase;

struct TaskPromiseBase {
    struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }
        template <typename PromiseType>
        auto await_suspend(std::experimental::coroutine_handle<PromiseType> h) noexcept {
            return h.promise().continuation;
        }
        void await_resume() noexcept {}
    };
    TaskBase get_return_object() noexcept;
    std::experimental::suspend_always initial_suspend() noexcept { return {}; }
    std::experimental::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}
    void return_void() noexcept {}

    std::experimental::coroutine_handle<> continuation;
};

struct TaskPromiseAlwaysElide : public TaskPromiseBase {
    static constexpr bool should_elide() {
        return true;
    }
};

struct TaskPromiseNeverElide : public TaskPromiseBase {
    static constexpr bool should_elide() {
        return false;
    }
};

class TaskBase {
public:
    TaskBase(std::experimental::coroutine_handle<> handle) : handle(handle) {}
    TaskBase(TaskBase&& t) : handle(std::exchange(t.handle, nullptr)) {}
    ~TaskBase() {
        if (handle && !handle.elided())
            handle.destroy();
    }

    bool Elided() const {
        return handle.elided();
    }
protected:
    std::experimental::coroutine_handle<> handle;
};

TaskBase TaskPromiseBase::get_return_object() noexcept {
    return std::experimental::coroutine_handle<TaskPromiseBase>::from_promise(*this);
}

template<class PromiseType>
class Task : public TaskBase{
public:
    using promise_type = PromiseType;
    using HandleType = std::experimental::coroutine_handle<promise_type>;
    Task(TaskBase&& Base) : TaskBase(std::move(Base)) {}

    struct Awaiter {
        HandleType handle;
        Awaiter(HandleType handle) : handle(handle) {}
        bool await_ready() const noexcept { return false; }
        __attribute__((__always_inline__))
        std::experimental::coroutine_handle<void> 
          await_suspend(std::experimental::coroutine_handle<void> continuation) noexcept {
            HandleType::from_address(handle.address()).promise().continuation = continuation;
            return handle;
        }
        void await_resume() noexcept {
            handle.destroy();
        }
    };

    auto operator co_await() {
        return Awaiter(std::exchange(handle, nullptr));
    }
};

using NormalTask = Task<TaskPromiseBase>;
using AlwaysElideTask = Task<TaskPromiseAlwaysElide>;
using NeverElideTask = Task<TaskPromiseNeverElide>;

struct ShouldElideTagT;
struct NoElideTagT;
struct MayElideTagT;

namespace detail {
    template< class T, class U >
    concept SameHelper = std::is_same_v<T, U>;
}
 
template< class T, class U >
concept same_as = detail::SameHelper<T, U> && detail::SameHelper<U, T>;

template<typename T>
concept ElideTag = (same_as<T, ShouldElideTagT> ||
                      same_as<T, NoElideTagT> ||
                      same_as<T, MayElideTagT>);

bool undetermism;

template <ElideTag Tag>
struct TaskPromiseAlternative : public TaskPromiseBase {
    static constexpr bool should_elide() {
        if constexpr (std::is_same_v<Tag, ShouldElideTagT>)
            return true;
        else if constexpr (std::is_same_v<Tag, NoElideTagT>)
            return false;
        else
            return undetermism;
    }
};

template <ElideTag Tag = MayElideTagT>
using AlternativeTask = Task<TaskPromiseAlternative<Tag>>;

NormalTask normal_task () {
    co_await std::experimental::suspend_always{};
}
AlwaysElideTask always_elide_task () {
    co_await std::experimental::suspend_always{};
}
NeverElideTask never_elide_task () {
    co_await std::experimental::suspend_always{};
}
AlternativeTask<> alternative_task_default () {
    co_await std::experimental::suspend_always{};
}
template <ElideTag Tag>
AlternativeTask<Tag> alternative_task () {
    co_await std::experimental::suspend_always{};
}

int main() {
    auto t = normal_task();
    std::cout << "Normal Task should not be elided. Is is elided? " << t.Elided() << "\n";
    auto t2 = always_elide_task();
    std::cout << "Always Elide Task should be elided.  Is is elided? " << t2.Elided() << "\n";
    auto t3 = never_elide_task();
    std::cout << "Never Elide Task should not be elided.  Is is elided? " << t3.Elided() << "\n";
    auto t4 = alternative_task_default();
    std::cout << "Default alternative task should not be elided.  Is is elided? " << t4.Elided() << "\n";
    auto t5 = alternative_task<ShouldElideTagT>();
    std::cout << "ShouldElideTagT should be elided. Is is elided? " << t5.Elided() << "\n";
    
    auto t6 = alternative_task<NoElideTagT>();
    std::cout << "NoElideTagT should not be elided. Is is elided? " << t6.Elided() << "\n";
}
