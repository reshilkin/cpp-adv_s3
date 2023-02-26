#pragma once

#include <array>
#include <concepts>
#include <exception>
#include <memory>

struct bad_variant_access : std::exception {
  const char* what() const noexcept override {
    return "bad variant access";
  }
};

static constexpr size_t variant_npos = static_cast<size_t>(-1);

template <typename T>
struct in_place_type_t {};

template <typename T>
static inline constexpr in_place_type_t<T> in_place_type;

template <size_t I>
struct in_place_index_t {};

template <size_t I>
static inline constexpr in_place_index_t<I> in_place_index;

namespace v_utils {

template <size_t I, typename... Types>
struct get_ith;

template <size_t I, typename Type, typename... Types>
struct get_ith<I, Type, Types...> : get_ith<I - 1, Types...> {};

template <typename Type, typename... Types>
struct get_ith<0, Type, Types...> {
  using type = Type;
};

template <size_t I>
struct get_ith<I> {
  using type = void;
};

template <size_t I, typename... Types>
using get_ith_t = typename get_ith<I, Types...>::type;

template <typename... Types>
concept trivially_destructible = std::conjunction_v<std::is_trivially_destructible<Types>...>;

template <typename... Types>
concept trivial_copy_constructible = std::conjunction_v<std::is_trivially_copy_constructible<Types>...>;

template <typename... Types>
concept trivial_copy_assignable = std::conjunction_v<std::is_trivially_copy_assignable<Types>...>;

template <typename... Types>
concept copy_constructible = std::conjunction_v<std::is_copy_constructible<Types>...>;

template <typename... Types>
concept copy_assignable = std::conjunction_v<std::is_copy_assignable<Types>...>;

template <typename... Types>
concept trivial_move_constructible = std::conjunction_v<std::is_trivially_move_constructible<Types>...>;

template <typename... Types>
concept trivial_move_assignable = std::conjunction_v<std::is_trivially_move_assignable<Types>...>;

template <typename... Types>
concept move_constructible = std::conjunction_v<std::is_move_constructible<Types>...>;

template <typename... Types>
concept move_assignable = std::conjunction_v<std::is_move_assignable<Types>...>;

template <typename T, typename... Types>
struct cnt;

template <typename T, typename Type, typename... Types>
struct cnt<T, Type, Types...> {
  static constexpr size_t value = (std::is_same_v<T, Type> ? 1 : 0) + cnt<T, Types...>::value;
};

template <typename T>
struct cnt<T> {
  static constexpr size_t value = 0;
};

template <typename T, typename... Types>
static constexpr size_t cnt_v = cnt<T, Types...>::value;

template <typename T, typename... Types>
struct find_pos;

template <typename T, typename Type, typename... Types>
struct find_pos<T, Type, Types...> {
  static constexpr size_t value = std::is_same_v<T, Type> ? 0 : (find_pos<T, Types...>::value + 1);
};

template <typename T>
struct find_pos<T> {
  static constexpr size_t value = 0;
};

template <typename T, typename... Types>
static constexpr size_t find_pos_v = find_pos<T, Types...>::value;

template <typename To, typename From>
concept valid_cast = requires(From from, std::array<To, 1> to) {
  to = {std::forward<From>(from)};
};

template <typename Cast_From, typename Type>
struct find_cast_one {
  static Type f(Type) requires valid_cast<Type, Cast_From>;
};

template <typename Cast_From, typename... Types>
struct find_cast : find_cast_one<Cast_From, Types>... {
  using find_cast_one<Cast_From, Types>::f...;
};

template <typename T, typename... Types>
using find_cast_t = decltype(find_cast<T, Types...>::f(std::declval<T>()));

template <typename T, size_t... Dims>
struct multi_array;

template <typename T, size_t F, size_t... R>
struct multi_array<T, F, R...> {
  template <typename... sizes>
  constexpr T const& get_data(size_t cur, sizes... rest) const {
    return data[cur].get_data(rest...);
  }
  std::array<multi_array<T, R...>, F> data;
};

template <typename T>
struct multi_array<T> {
  constexpr T const& get_data() const {
    return data;
  }
  T data;
};

template <bool indexed, typename Stored, typename Indexes>
struct visit_table;

template <bool indexed, typename Return_Type, typename Visitor, typename... Variants, size_t Cur_Size,
          size_t... Rest_Sizes, size_t... Indexes>
struct visit_table<indexed, multi_array<Return_Type (*)(Visitor, Variants...), Cur_Size, Rest_Sizes...>,
                   std::index_sequence<Indexes...>> {
  using stored_t = multi_array<Return_Type (*)(Visitor, Variants...), Cur_Size, Rest_Sizes...>;

  static constexpr stored_t construct() {
    return construct_impl(std::make_index_sequence<Cur_Size>());
  }

  template <size_t... Next_Indexes>
  static constexpr stored_t construct_impl(std::index_sequence<Next_Indexes...>) {
    return {(visit_table<indexed, multi_array<Return_Type (*)(Visitor, Variants...), Rest_Sizes...>,
                         std::index_sequence<Indexes..., Next_Indexes>>::construct())...};
  }

  static constexpr stored_t stored = construct();
};

template <typename Return_Type, typename Visitor, typename... Variants, size_t... Indexes>
struct visit_table<false, multi_array<Return_Type (*)(Visitor, Variants...)>, std::index_sequence<Indexes...>> {
  using stored_t = multi_array<Return_Type (*)(Visitor, Variants...)>;

  static constexpr Return_Type invoke(Visitor&& vis, Variants&&... vars) {
    return std::forward<Visitor>(vis)(get_impl<Indexes>(std::forward<Variants>(vars))...);
  }

  static constexpr stored_t construct() {
    return {&invoke};
  }

  static constexpr stored_t stored = construct();
};

template <typename Return_Type, typename Visitor, typename... Variants, size_t... Indexes>
struct visit_table<true, multi_array<Return_Type (*)(Visitor, Variants...)>, std::index_sequence<Indexes...>> {
  using stored_t = multi_array<Return_Type (*)(Visitor, Variants...)>;

  static constexpr Return_Type invoke(Visitor&& vis, Variants&&... vars) {
    return std::forward<Visitor>(vis)(get_impl<Indexes>(std::forward<Variants>(vars))..., in_place_index<Indexes>...);
  }

  static constexpr stored_t construct() {
    return {&invoke};
  }

  static constexpr stored_t stored = construct();
};

} // namespace v_utils
