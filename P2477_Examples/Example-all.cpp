// clang++ -std=c++20 -O3
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

NormalTask example1(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    auto t = an_elided_coroutine();
    sum += co_await std::move(t);
  }

  co_return sum;
}

ControllableTask an_elide_controllable_coroutine(std::coro_elision elision_state = std::coro_elision::may) {
    co_return 43;
}

NormalTask example2(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    auto t = an_elide_controllable_coroutine(std::coro_elision::always);
    sum += co_await std::move(t);
  }

  co_return sum;
}

NormalTask example3(int count, std::coro_elision elision_state) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    // We couldn't control the elision in the form. Since the compiler
    // might not be able to know the value of elision_state in compilation
    // time.
    auto t = an_elide_controllable_coroutine(elision_state);
    sum += co_await std::move(t);
  }

  co_return sum;
}

template <ElisionTag Tag>
NormalTask example4(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    auto t = an_elide_controllable_coroutine(getStdCoroElision<Tag>());
    sum += co_await std::move(t);
  }

  co_return sum;
}

template <ElisionTag Tag>
TemplateControllableTask<Tag> another_elide_controllable_coroutine() {
  co_return 43;
}

template <ElisionTag Tag>
TemplateControllableTask<Tag> example5(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    // When we want to control the elision for a chain, it is better
    // to use following form.
    auto t = another_elide_controllable_coroutine<Tag>();
    sum += co_await std::move(t);
  }

  co_return sum;
}

int main() {
    allocated_counts = 0;
    std::cout << "Result of example1: " << example1(5).start() << "\n";
    // Only the NormalTask example1 may be allocated.
    std::cout << "allocated count: " << allocated_counts << "\n";
    assert(allocated_counts <= 1);

    allocated_counts = 0;
    std::cout << "Result of example2: " << example2(5).start() << "\n";
    // Only the NormalTask example2 may be allocated.
    std::cout << "allocated count: " << allocated_counts << "\n";
    assert(allocated_counts <= 1);

    allocated_counts = 0;
    // We couldn't control coroutine elision in the way.
    // See the comments in example3 for details.
    std::cout << "Result of example3: " << example3(5, std::coro_elision::always).start() << "\n";
    std::cout << "allocated count: " << allocated_counts << "\n";
    assert(allocated_counts <= 6);

    allocated_counts = 0;
    // Proper way to control coroutine elision in chain.
    std::cout << "Result of example4: " << example4<AlwaysElide>(5).start() << "\n";
    std::cout << "allocated count: " << allocated_counts << "\n";
    assert(allocated_counts <= 1);

    allocated_counts = 0;
    // Proper way to control coroutine elision in chain.
    std::cout << "Result of example4, never elide: " << example4<NeverElide>(5).start() << "\n";
    std::cout << "allocated count: " << allocated_counts << "\n";
    assert(allocated_counts >= 5);

    allocated_counts = 0;
    // Another way to control coroutine elision in chain.
    std::cout << "Result of example5: " << example5<AlwaysElide>(5).start() << "\n";
    std::cout << "allocated count: " << allocated_counts << "\n";
    assert(allocated_counts <= 1);   

    allocated_counts = 0;
    // Another way to control coroutine elision in chain.
    std::cout << "Result of example5, never elide: " << example5<NeverElide>(5).start() << "\n";
    std::cout << "allocated count: " << allocated_counts << "\n";
    assert(allocated_counts >= 5);
    return 0;
}
