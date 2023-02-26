#ifndef OPTIONAL_ENABLE_BASES_H
#include <type_traits>

namespace enable {

template <typename T, bool enabled = std::is_copy_assignable_v<T>&&
                          std::is_copy_constructible_v<T>>
struct copy_assign_base {
  constexpr copy_assign_base() = default;
  constexpr copy_assign_base(copy_assign_base const&) = default;
  constexpr copy_assign_base(copy_assign_base&&) = default;
  constexpr copy_assign_base& operator=(copy_assign_base const&) = delete;
  constexpr copy_assign_base& operator=(copy_assign_base&&) = default;
};

template <typename T>
struct copy_assign_base<T, true> {};

template <typename T, bool enabled = std::is_move_assignable_v<T>&&
                          std::is_move_constructible_v<T>>
struct move_assign_base {
  constexpr move_assign_base() = default;
  constexpr move_assign_base(move_assign_base const&) = default;
  constexpr move_assign_base(move_assign_base&&) = default;
  constexpr move_assign_base& operator=(move_assign_base const&) = default;
  constexpr move_assign_base& operator=(move_assign_base&&) = delete;
};

template <typename T>
struct move_assign_base<T, true> {};

template <typename T, bool enabled = std::is_copy_constructible_v<T>>
struct copy_construct_base {
  constexpr copy_construct_base() = default;
  constexpr copy_construct_base(copy_construct_base&&) = default;
  constexpr copy_construct_base(copy_construct_base const&) = delete;
  constexpr copy_construct_base&
  operator=(copy_construct_base const&) = default;
  constexpr copy_construct_base& operator=(copy_construct_base&&) = default;
};

template <typename T>
struct copy_construct_base<T, true> {};

template <typename T, bool enabled = std::is_move_constructible_v<T>>
struct move_construct_base {
  constexpr move_construct_base() = default;
  constexpr move_construct_base(move_construct_base const&) = default;
  constexpr move_construct_base(move_construct_base&&) = delete;
  constexpr move_construct_base&
  operator=(move_construct_base const&) = default;
  constexpr move_construct_base& operator=(move_construct_base&&) = default;
};

template <typename T>
struct move_construct_base<T, true> {};

} // namespace enable
#define OPTIONAL_ENABLE_BASES_H

#endif // OPTIONAL_ENABLE_BASES_H
