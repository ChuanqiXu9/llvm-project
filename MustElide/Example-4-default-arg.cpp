// clang++ -std=c++2a -O3 ShouldElide.cpp
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

NormalTask has_default_args(AlwaysElideTask t = an_elided_coroutine()) {
  assert (t.Elided());
  co_return co_await std::move(t);
}

NormalTask example4() {
  NormalTask t = has_default_args(); // Implicit call to an_elided_coroutine() here.
                                     // Its storage-lifetime ends at the end of the
                                     // enclosing block-scope.
  co_return (co_await std::move(t)) + 1;
}

int main() {
    std::cout << "Result: " << example4().start() << "\n";
    return 0;
}