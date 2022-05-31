// clang++ -std=c++20 -O3 -I.. -pthread
#include "Task.h"

#include <iostream>

Task foo() {
    co_return 43;
}

Task bar(std::size_t iter_count) {
    int sum = 0;
    for (std::size_t i = 0; i < iter_count; i++)
        sum += co_await foo();
    co_return sum;
}

Task foo_must_elide(std::coro_elision elision_state = std::coro_elision::may) {
    co_return 43;
}

Task bar_must_elide(std::size_t iter_count, std::coro_elision elision_state = std::coro_elision::may) {
    int sum = 0;
    for (std::size_t i = 0; i < iter_count; i++)
        sum += co_await foo_must_elide(std::coro_elision::always);
    co_return sum;
}

int main() {
    allocated_counts = 0;
    SimpleExecutor E(4);
    // Asynchoronous task couldn't be elided by compiler.
    std::cout << "Original Result: " << bar(5).via(&E).syncStart() << "\n";
    assert(allocated_counts == 6);

    allocated_counts = 0;
    // We could control the coroutine elision explcitly in this manner.
    std::cout << "Original Result: " << bar_must_elide(5, std::coro_elision::always).via(&E).syncStart() << "\n";
    assert(allocated_counts == 0);

    return 0;
}
