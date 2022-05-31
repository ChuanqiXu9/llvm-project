// clang++ -std=c++20 -O3
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

NormalTask has_default_args(AlwaysElideTask t = an_elided_coroutine()) {
  co_return co_await std::move(t);
}

NormalTask example4() {
  NormalTask t = has_default_args(); // Implicit call to an_elided_coroutine() here.
                                     // Its storage-lifetime ends at the end of the
                                     // enclosing block-scope.
  co_return (co_await std::move(t)) + 1;
}

int main() {
    allocated_counts = 0;
    std::cout << "Result: " << example4().start() << "\n";
    assert(allocated_counts <= 2);
    return 0;
}