// clang++ -std=c++2a -O3 ShouldElide.cpp
#include "TaskBase.h"
#include <iostream>

// Test that if the compiler could use TaskPromiseDerived1::must_elide()
struct TaskPromiseDerived1 : public TaskPromiseAlwaysElide {
    static constexpr bool must_elide() {
        return false;
    }
};

// Test that if the compiler could found TaskPromiseAlwaysElide::must_elide()
struct TaskPromiseDerived2 : public TaskPromiseAlwaysElide {};

using Task1 = Task<TaskPromiseDerived1>;
using Task2 = Task<TaskPromiseDerived2>;

Task1 task1() {
    co_await std::experimental::suspend_always{};
    co_return 43;
}

Task2 task2() {
    co_await std::experimental::suspend_always{};
    co_return 43;
}

int main() {
    auto t1 = task1();
    assert (!t1.Elided());
    std::cout << "t1 wouldn't be elided.\n";
    auto t2 = task2();
    assert (t2.Elided());
    std::cout << "t2 would be elided.\n";
}
