// clang++ -std=c++20 -O3
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

struct S {
  AlwaysElideTask task = an_elided_coroutine();
};

NormalTask example6() {
  S s{}; // an_elided_coroutine() evaluated in the context of example7()
  int x = co_await std::move(s.task); // OK

  co_return x;
}

int main() {
    allocated_counts = 0;
    std::cout << "Result: " << example6().start() << "\n";
    assert(allocated_counts <= 1);
    return 0;
}