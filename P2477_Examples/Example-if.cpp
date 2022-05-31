// clang++ -std=c++20 -O3
// This example is expeceted to fail
#include "TaskBase.h"
#include <iostream>

AlwaysElideTask an_elided_coroutine() {
    co_return 42;
}

AlwaysElideTask another_elided_coroutine() {
    co_return 43;
}

NormalTask example2(bool cond) {
  AlwaysElideTask t = cond ? an_elided_coroutine() : another_elided_coroutine();
  co_return co_await std::move(t);
}

// We couldn't do following
//
// ```
//  NormalTask example2(bool cond) {
//    AlwaysElideTask t ;
//
//    if (cond)
//        t = an_elided_coroutine();
//    else
//        t = another_elided_coroutine();
//    
//    co_return co_await std::move(t);
//  }
// ```
//
// There are 2 reasons:
// 1. AlwaysElideTask isn't default constructible nor copy constructible.
// 2. After eliding, the lifetimes of an_elided_coroutine() and
//    another_elided_coroutine() are restricted to the branches
//    of the if statement. So it would crash if we try to resume t.
int main() {
    allocated_counts = 0;
    std::cout << "Result: " << example2(true).start() << "\n";
    assert(allocated_counts <= 1);
    return 0;
}
