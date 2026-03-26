// This file tests the coro_await_elidable attribute semantics.
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

template <typename T>
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
      return std::coroutine_handle<promise_type>::from_promise(*this);
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
    void return_value(T x) noexcept {
      value = x;
    }

    std::coroutine_handle<> continuation;
    T value;
  };

  Task(std::coroutine_handle<promise_type> handle) : handle(handle) {}
  ~Task() {
    if (handle)
      handle.destroy();
  }

  struct Awaiter {
    Awaiter(Task *t) : task(t) {}
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<void> continuation) noexcept {}
    T await_resume() noexcept {
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

class Dtor {
public:
    Dtor();
    ~Dtor();
};

Task<Dtor> callee() {
  co_return {};
}

Dtor may_throw();

Task<Dtor> elidable(int limit) {
    for (int i = 0; i < limit; ++i) {
        auto t = co_await callee();

        auto t2 = may_throw();
        auto t3 = may_throw();
        auto t4 = may_throw();
    }

    co_return {};
}

// CHECK-FRONTEND: call {{.*}}@llvm.coro.stacked.allocator(ptr {{.*}}%__promise, ptr {{.*}}@_ZN4TaskI4DtorE12promise_type16stacked_allocateEmSt11align_val_t, ptr {{.*}}@_ZN4TaskI4DtorE12promise_type18stacked_deallocateEPvSt11align_val_t

// One for normal path and one for exceptional path.
// CHECK-LABEL: define {{.*}}@_Z8elidablei.resume
// CHECK: call {{.*}}@_Z23stacked_deallocate_implPvSt11align_val_t
// CHECK: call {{.*}}@_Z23stacked_deallocate_implPvSt11align_val_t

// CHECK-LABEL: define {{.*}}@_Z8elidablei.destroy
// CHECK: call {{.*}}@_Z23stacked_deallocate_implPvSt11align_val_t
