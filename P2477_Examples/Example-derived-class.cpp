// clang++ -std=c++20 -O3
#include "TaskBase.h"
#include <iostream>

// Test that if the compiler could use TaskPromiseDerived1::must_elide()
struct TaskPromiseDerived1 : public TaskPromiseAlwaysElide {
    static constexpr std::coro_elision must_elide() {
        return std::coro_elision::never;
    }
};

// Test that if the compiler could found TaskPromiseAlwaysElide::must_elide()
struct TaskPromiseDerived2 : public TaskPromiseAlwaysElide {};

using Task1 = Task<TaskPromiseDerived1>;
using Task2 = Task<TaskPromiseDerived2>;

Task1 task1() {
    co_await std::suspend_always{};
    co_return 43;
}

Task2 task2() {
    co_await std::suspend_always{};
    co_return 43;
}

int main() {
    allocated_counts = 0;
    auto t1 = task1();
    std::cout << "t1 wouldn't be elided.\n";
    assert(allocated_counts == 1);
    auto t2 = task2();
    std::cout << "t2 would be elided.\n";
     assert(allocated_counts == 1);
}