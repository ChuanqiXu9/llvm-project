// clang++ -std=c++2a -O3 ShouldElide.cpp
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

NormalTask example1(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i)
    sum += co_await an_elided_coroutine();

  co_return sum;
}

int main() {
    std::cout << "Result: " << example1(5).start() << "\n";
    return 0;
}