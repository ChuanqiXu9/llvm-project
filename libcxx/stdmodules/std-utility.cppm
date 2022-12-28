module;
#include <utility>
# 1 __FILE__ 1 3
export module std:utility;
export namespace std {
    using std::swap;

    namespace rel_ops {
        using rel_ops::operator!=;
        using rel_ops::operator>;
        using rel_ops::operator<=;
        using rel_ops::operator>=;
    }

    using std::forward;
    using std::forward_like;
    using std::move;
    using std::move_if_noexcept;
    using std::as_const;
    using std::declval;
    using std::cmp_equal;
    using std::cmp_not_equal;     // C++20
    using std::cmp_less;          // C++20
    using std::cmp_greater;       // C++20
    using std::cmp_less_equal;    // C++20
    using std::cmp_greater_equal; // C++20
    using std::in_range;          // C++20

    using std::pair;
    using std::operator==;

    using std::make_pair;
    using std::piecewise_construct_t;
    // FIXME: We can't export non-inline constexpr variables.
    // using std::piecewise_construct;

    using std::tuple_size;
    using std::tuple_element;

    using std::get;
    using std::integer_sequence;
    using std::index_sequence;
    using std::make_index_sequence;
    using std::make_integer_sequence;
    using std::index_sequence_for;

    using std::exchange;
    using std::in_place_t;
    using std::in_place;

    using std::in_place_type_t;
    using std::in_place_type;
    using std::in_place_index_t;

    using std::in_place_index;
    using std::to_underlying;
}
