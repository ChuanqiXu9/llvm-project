// clang++ -std=c++2a -O3 ShouldElide.cpp
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

NormalTask example5(bool cond) {
  int result;
  if (auto t = an_elided_coroutine(); cond) {
    assert (t.Elided());
    result = co_await std::move(t);
    result += 1;
  } else {
    assert (t.Elided());
    result = co_await std::move(t);
    result += 2;
  } // coroutine storage lifetime ends here

  co_return result;
}

int main() {
    std::cout << "Result for true: " << example5(true).start() << "\n";
    std::cout << "Result for true: " << example5(false).start() << "\n";
    return 0;
}