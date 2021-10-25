#include <type_traits>
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
    FinalAwaiter final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}

    void return_value(int x) noexcept {
        _value = x;
    }

    std::experimental::coroutine_handle<> continuation = std::experimental::noop_coroutine();
    int _value;
};

struct TaskPromiseAlwaysElide : public TaskPromiseBase {
    static constexpr bool must_elide() {
        return true;
    }
};

struct TaskPromiseNeverElide : public TaskPromiseBase {
    static constexpr bool must_elide() {
        return false;
    }
};

class TaskBase {
public:
    TaskBase(std::experimental::coroutine_handle<> handle) : handle(handle) {}
    TaskBase(TaskBase&& t) : handle(std::exchange(t.handle, nullptr)) {}
    ~TaskBase() {
        if (handle)
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
       template <typename U>
        auto await_suspend(std::experimental::coroutine_handle<U> continuation) noexcept {
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
    static constexpr bool must_elide() {
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
