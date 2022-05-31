// clang++ -std=c++20 -O3
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

NormalTask example5(bool cond) {
  int result;
  if (auto t = an_elided_coroutine(); cond) {
    result = co_await std::move(t);
    result += 1;
  } else {
    result = co_await std::move(t);
    result += 2;
  } // coroutine storage lifetime ends here

  co_return result;
}

int main() {
    allocated_counts = 0;
    std::cout << "Result for true: " << example5(true).start() << "\n";
    std::cout << "Result for true: " << example5(false).start() << "\n";
    assert(allocated_counts <= 2);
    return 0;
}
