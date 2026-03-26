// This file tests the stacked allocator in CoroElide pass.
// RUN: %clang_cc1 -triple=x86_64-unknown-linux-gnu -fexceptions -fcxx-exceptions \
// RUN:     -std=c++20 -O3 -emit-llvm %s -disable-llvm-passes -o - | FileCheck %s --check-prefix=CHECK-FRONTEND
// RUN: %clang_cc1 -triple=x86_64-unknown-linux-gnu -fexceptions -fcxx-exceptions \
// RUN:     -std=c++20 -O3 -emit-llvm %s -o - | FileCheck %s

#include "Inputs/coroutine.h"
#include "Inputs/utility.h"

namespace std {
    typedef __SIZE_TYPE__ size_t;
    enum class align_val_t : size_t {};
}

void *stacked_allocate_impl(std::size_t size, std::align_val_t align);
void stacked_deallocate_impl(void *ptr, std::align_val_t align);

struct [[clang::coro_await_elidable]] Task {
    struct promise_type {
        struct FinalAwaiter {
            bool await_ready() const noexcept { return false; }

            template <typename P>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<P> coro) noexcept {
                if (!coro)
                    return std::noop_coroutine();
                return coro.promise().continuation;
            }
            void await_resume() noexcept {}
        };

        Task get_return_object() noexcept {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        void *stacked_allocate(std::size_t size, std::align_val_t align) {
            return stacked_allocate_impl(size, align);
        }
        void stacked_deallocate(void *ptr, std::align_val_t align) {
            stacked_deallocate_impl(ptr, align);
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        FinalAwaiter final_suspend() noexcept { return {}; }
        void unhandled_exception() noexcept {}
        void return_value(int x) noexcept {
            value = x;
        }

        std::coroutine_handle<> continuation;
        int value;
    };

    Task(std::coroutine_handle<promise_type> handle) : handle(handle) {}
    ~Task() {
        if (handle)
            handle.destroy();
    }

    struct Awaiter {
        Awaiter(Task *t) : task(t) {}
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<void> continuation) noexcept {
            task->handle.promise().continuation = continuation;
        }
        int await_resume() noexcept {
            return task->handle.promise().value;
        }

        Task *task;
    };

    auto operator co_await() {
        return Awaiter{this};
    }

private:
    std::coroutine_handle<promise_type> handle;
};

Task foo() {
    co_return 43;
}

Task bar() {
    int v = co_await foo();
    co_return v;
}

Task nested() {
    int v1 = co_await foo();
    int v2 = co_await foo();
    co_return v1 + v2;
}

// CHECK-FRONTEND: call {{.*}}@llvm.coro.stacked.allocator(ptr {{.*}}%__promise, ptr {{.*}}@_ZN4Task12promise_type16stacked_allocateEmSt11align_val_t, ptr {{.*}}@_ZN4Task12promise_type18stacked_deallocateEPvSt11align_val_t

// Verify that stacked_deallocate_impl is called in the resume and destroy functions
// CHECK-LABEL: define {{.*}}@_Z3barv.resume
// CHECK: call {{.*}}@_Z23stacked_deallocate_implPvSt11align_val_t

// CHECK-LABEL: define {{.*}}@_Z3barv.destroy
// CHECK: call {{.*}}@_Z23stacked_deallocate_implPvSt11align_val_t

// CHECK-LABEL: define {{.*}}@_Z6nestedv.resume
// CHECK: call {{.*}}@_Z23stacked_deallocate_implPvSt11align_val_t

// CHECK-LABEL: define {{.*}}@_Z6nestedv.destroy
// CHECK: call {{.*}}@_Z23stacked_deallocate_implPvSt11align_val_t