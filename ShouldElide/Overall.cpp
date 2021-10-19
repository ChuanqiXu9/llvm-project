// clang++ -std=c++2a -O3 ShouldElide.cpp
#include <TaskBase.h>
#include <iostream>

NormalTask normal_task () {
    co_await std::experimental::suspend_always{};
    co_return 43;
}
AlwaysElideTask always_elide_task () {
    co_await std::experimental::suspend_always{};
    co_return 43;
}
NeverElideTask never_elide_task () {
    co_await std::experimental::suspend_always{};
    co_return 43;
}
AlternativeTask<> alternative_task_default () {
    co_await std::experimental::suspend_always{};
    co_return 43;
}
template <ElideTag Tag>
AlternativeTask<Tag> alternative_task () {
    co_await std::experimental::suspend_always{};
    co_return 43;
}

int main() {
    auto t = normal_task();
    std::cout << "Normal Task should not be elided. Is it elided? " << t.Elided() << "\n";
    auto t2 = always_elide_task();
    std::cout << "Always Elide Task should be elided.  Is it elided? " << t2.Elided() << "\n";
    auto t3 = never_elide_task();
    std::cout << "Never Elide Task should not be elided.  Is it elided? " << t3.Elided() << "\n";
    auto t4 = alternative_task_default();
    std::cout << "Default alternative task should not be elided.  Is it elided? " << t4.Elided() << "\n";
    auto t5 = alternative_task<ShouldElideTagT>();
    std::cout << "ShouldElideTagT should be elided. Is it elided? " << t5.Elided() << "\n";

    auto t6 = alternative_task<NoElideTagT>();
    std::cout << "NoElideTagT should not be elided. Is it elided? " << t6.Elided() << "\n";
}
