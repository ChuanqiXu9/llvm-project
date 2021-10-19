// clang++ -std=c++2a -O3 ShouldElide.cpp
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}


NormalTask example3(int count) {
  int sum = 0;
retry:
  AlwaysElideTask t = an_elided_coroutine();
  sum += co_await std::move(t);
  if (sum < count) goto retry; // <- goto exits block-scope of coroutine invocation
  co_return sum;              //    ending storage lifetime of the coroutine.
}

int main() {
    std::cout << "Result: " << example3(5).start() << "\n";
    return 0;
}