// clang++ -std=c++2a -O3 ShouldElide.cpp
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

NormalTask example1(int count) {
  int sum = 0;
<<<<<<< HEAD
  for (int i = 0; i < count; ++i)
    sum += co_await an_elided_coroutine();
=======
  for (int i = 0; i < count; ++i) {
    auto t = an_elided_coroutine();
    assert (t.Elided());
    sum += co_await std::move(t);
  }
>>>>>>> Updating tests and fix bugs in impl

  co_return sum;
}

int main() {
    std::cout << "Result: " << example1(5).start() << "\n";
    return 0;
}