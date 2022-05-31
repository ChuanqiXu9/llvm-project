// clang++ -std=c++20 -O3
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
    allocated_counts = 0;
    std::cout << "Result: " << example3(5).start() << "\n";
    assert(allocated_counts <= 1);
    return 0;
}
