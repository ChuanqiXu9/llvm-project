module;
#include <coroutine>
# 1 __FILE__ 1 3
export module std:coroutine;
export namespace std {
    using std::coroutine_traits;
    using std::coroutine_handle;
    using std::operator==;
    using std::operator<=>;
    using std::noop_coroutine;
    using std::suspend_never;
    using std::suspend_always;

    using std::hash;
}
