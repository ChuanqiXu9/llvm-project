// clang++ -std=c++2a -O3 ShouldElide.cpp
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
  assert (s.task.Elided());
  int x = co_await std::move(s.task); // OK

  co_return x;
}

int main() {
    std::cout << "Result: " << example6().start() << "\n";
    return 0;
}