module;
#include <exception>
# 1 __FILE__ 1 3
export module std:exception;
export namespace  std {
    using std::exception;
    using std::bad_exception;
    using std::set_terminate;
    using std::get_terminate;
    using std::terminate;
    using std::uncaught_exception;
    using std::exception_ptr;
    using std::current_exception;
    using std::make_exception_ptr;
    using std::nested_exception;
    using std::throw_with_nested;
    using std::rethrow_if_nested;
}
