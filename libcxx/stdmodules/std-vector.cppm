module;
#include <vector>
# 1 __FILE__ 1 3
export module std:vector;
export namespace std {
    using std::vector;
    using std::hash;
    using std::swap;
    using std::operator==;
    using std::erase;
    using std::erase_if;
}

export {
    using ::operator new;
}
