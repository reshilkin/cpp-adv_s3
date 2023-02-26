#pragma once

#include "variant-storage.h"
#include "variant_utils.h"
#include <functional>

template <typename... Types>
struct variant;

template <size_t I, typename T>
struct variant_alternative;

template <size_t I, typename... Types>
struct variant_alternative<I, variant<Types...>> {
  using type = v_utils::get_ith_t<I, Types...>;
};

template <size_t I, typename T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

template <size_t I, typename T>
struct variant_alternative<I, const T> {
  using type = std::add_const_t<variant_alternative_t<I, T>>;
};

template <typename T, typename Variant>
struct alternative_ind;

template <typename T, typename... Types>
struct alternative_ind<T, variant<Types...>> {
  static constexpr size_t value = v_utils::find_pos_v<T, Types...>;
};

template <typename T, typename V>
constexpr inline size_t alternative_ind_v = alternative_ind<T, V>::value;

template <typename T, typename Variant>
struct alternative_ind<T, const Variant> : alternative_ind<T, Variant> {};

template <typename T>
struct variant_size;

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename T>
struct variant_size<const T> : variant_size<T> {};

template <typename T>
constexpr inline size_t variant_size_v = variant_size<T>::value;

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&&, Variants&&...);

namespace v_utils {

template <typename R, typename Visitor, typename... Variants>
constexpr R indexed_visit(Visitor&&, Variants&&...);

}

template <typename T, typename... Types>
constexpr bool holds_alternative(const variant<Types...>&) noexcept;

template <size_t I, typename Var>
constexpr decltype(auto) get(Var&&);

template <typename T, typename Var>
constexpr decltype(auto) get(Var&&);

template <typename... Types>
struct variant : v_storage::variant_storage<Types...> {

private:
  using base = v_storage::variant_storage<Types...>;
  using base::ind;
  using base::reset_storage;

public:
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<v_utils::get_ith_t<0, Types...>>)
      requires(std::is_default_constructible_v<v_utils::get_ith_t<0, Types...>>)
      : base(in_place_index<0>) {}

  constexpr variant(variant const& other) noexcept(false)
      requires(v_utils::copy_constructible<Types...> && !v_utils::trivial_copy_constructible<Types...>)
      : base() {
    construct_from_variant(other);
  }

  constexpr variant(variant const& other) requires(v_utils::trivial_copy_constructible<Types...>) = default;

  constexpr variant(variant&& other) noexcept((std::is_nothrow_move_constructible_v<Types> && ...))
      requires(v_utils::move_constructible<Types...> && !v_utils::trivial_move_constructible<Types...>)
      : base() {
    construct_from_variant(std::move(other));
  }

  constexpr variant(variant&& other) requires(v_utils::trivial_move_constructible<Types...>) = default;

  template <typename T, typename X = v_utils::find_cast_t<T, Types...>>
  constexpr variant(T&& x) noexcept(std::is_nothrow_constructible_v<X, T>)
      : base(in_place_index<v_utils::find_pos_v<X, Types...>>, std::forward<T>(x)) {}

  template <typename T, typename... Args>
  constexpr variant(in_place_type_t<T>, Args&&... args)
      requires(v_utils::cnt_v<T, Types...> == 1 && std::is_constructible_v<T, Args...>)
      : base(in_place_index<v_utils::find_pos_v<T, Types...>>, std::forward<Args>(args)...) {}

  template <size_t I, typename... Args>
  constexpr variant(in_place_index_t<I>, Args&&... args)
      requires(I < sizeof...(Types) && std::is_constructible_v<v_utils::get_ith_t<I, Types...>, Args...>)
      : base(in_place_index<I>, std::forward<Args>(args)...) {}

  constexpr ~variant() = default;

  constexpr variant& operator=(variant const& other)
      requires(v_utils::copy_constructible<Types...>&& v_utils::copy_assignable<Types...> &&
               (!v_utils::trivial_copy_constructible<Types...> || !v_utils::trivial_copy_assignable<Types...> ||
                !v_utils::trivially_destructible<Types...>)) {
    if (other.valueless_by_exception()) {
      reset_storage();
    } else {
      v_utils::indexed_visit<void>(
          [this, &other]<typename Type, size_t other_pos>(Type& other_value, in_place_index_t<other_pos>) {
            if (ind == other_pos) {
              get<other_pos>(*this) = other_value;
            } else {
              if constexpr (std::is_nothrow_copy_constructible_v<Type> || !std::is_nothrow_move_constructible_v<Type>) {
                emplace<other_pos>(other_value);
              } else {
                *this = variant(other);
              }
            }
          },
          other);
    }
    return *this;
  }

  constexpr variant& operator=(variant const& other)
      requires(v_utils::trivial_copy_constructible<Types...>&& v_utils::trivial_copy_assignable<Types...>&&
                   v_utils::trivially_destructible<Types...>) = default;

  constexpr variant& operator=(variant&& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                                           std::is_nothrow_move_assignable_v<Types>)&&...))
      requires(v_utils::move_constructible<Types...>&& v_utils::move_assignable<Types...> &&
               (!v_utils::trivial_move_constructible<Types...> || !v_utils::trivial_move_assignable<Types...> ||
                !v_utils::trivially_destructible<Types...>)) {
    if (other.valueless_by_exception()) {
      reset_storage();
    } else {
      v_utils::indexed_visit<void>(
          [this]<typename Type, size_t other_pos>(Type&& other_value, in_place_index_t<other_pos>) {
            if (ind == other_pos) {
              get<other_pos>(*this) = std::forward<Type>(other_value);
            } else {
              emplace<other_pos>(std::forward<Type>(other_value));
            }
          },
          std::move(other));
    }
    return *this;
  }

  constexpr variant& operator=(variant&& other)
      requires(v_utils::trivial_move_constructible<Types...>&& v_utils::trivial_move_assignable<Types...>&&
                   v_utils::trivially_destructible<Types...>) = default;

  template <typename T, typename X = v_utils::find_cast_t<T, Types...>>
  constexpr variant&
  operator=(T&& t) noexcept(std::is_nothrow_assignable_v<X&, T>&& std::is_nothrow_constructible_v<X, T>)
      requires(std::is_assignable_v<X&, T>&& std::is_constructible_v<X, T>) {
    if (holds_alternative<X>(*this)) {
      ::visit<void>(
          [&]<typename Type>(Type&& value) {
            if constexpr (std::is_same_v<std::remove_cvref_t<Type>, std::remove_cvref_t<X>>) {
              std::forward<Type>(value) = std::forward<T>(t);
            }
          },
          *this);
    } else if (std::is_nothrow_constructible_v<X, T> || !std::is_nothrow_move_constructible_v<X>) {
      emplace<X>(std::forward<T>(t));
    } else {
      emplace<X>(X(std::forward<T>(t)));
    }
    return *this;
  }

  constexpr size_t index() const noexcept {
    return ind;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index() == variant_npos;
  }

  template <typename T, typename... Args>
  constexpr T& emplace(Args&&... args) {
    return emplace<v_utils::find_pos_v<T, Types...>>(std::forward<Args>(args)...);
  }

  template <size_t I, typename... Args>
  constexpr variant_alternative_t<I, variant>& emplace(Args&&... args) {
    reset_storage();
    std::construct_at<v_utils::get_ith_t<I, Types...>>(std::addressof(v_storage::get_impl<I>(*this)),
                                                       std::forward<Args>(args)...);
    ind = I;
    return get<I>(*this);
  }

  constexpr void swap(variant& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                                 std::is_nothrow_swappable_v<Types>)&&...)) {
    if (index() == other.index()) {
      if (index() == variant_npos) {
        return;
      }
      v_utils::indexed_visit<void>(
          [this]<typename Type, size_t other_pos>(Type&& other_value, in_place_index_t<other_pos>) {
            using std::swap;
            swap(v_storage::get_impl<other_pos>(*this), std::forward<Type>(other_value));
          },
          other);
    } else {
      variant tmp(std::move(other));
      other = std::move(*this);
      *this = std::move(tmp);
    }
  }

private:
  template <typename Other>
  void construct_from_variant(Other&& other) {
    ind = variant_npos;
    if (other.index() != variant_npos) {
      v_utils::indexed_visit<void>(
          [this]<typename Type, size_t other_pos>(Type&& other_value, in_place_index_t<other_pos>) {
            std::construct_at<std::remove_reference_t<Type>>(std::addressof(v_storage::get_impl<other_pos>(*this)),
                                                             std::forward<Type>(other_value));
            ind = other_pos;
          },
          std::forward<Other>(other));
    }
  }
};

template <size_t I, typename Var>
constexpr decltype(auto) get(Var&& v) {
  if (v.index() != I) {
    throw bad_variant_access();
  }
  return v_storage::get_impl<I>(std::forward<Var>(v));
}

template <typename T, typename Var>
constexpr decltype(auto) get(Var&& v) {
  return get<alternative_ind_v<T, std::remove_reference_t<Var>>>(std::forward<Var>(v));
}

template <size_t I, typename... Types>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Types...>>> get_if(variant<Types...>* pv) noexcept {
  if (pv != nullptr && pv->index() == I) {
    return std::addressof(get<I>(*pv));
  }
  return nullptr;
}

template <size_t I, typename... Types>
constexpr std::add_pointer_t<const variant_alternative_t<I, variant<Types...>>>
get_if(const variant<Types...>* pv) noexcept {
  if (pv != nullptr && pv->index() == I) {
    return std::addressof(get<I>(*pv));
  }
  return nullptr;
}

template <typename T, typename... Types>
constexpr std::add_pointer_t<T> get_if(variant<Types...>* pv) noexcept {
  return get_if<v_utils::find_pos_v<T, Types...>>(pv);
}
template <typename T, typename... Types>
constexpr std::add_pointer_t<const T> get_if(const variant<Types...>* pv) noexcept {
  return get_if<v_utils::find_pos_v<T, Types...>>(pv);
}

template <typename T, typename... Types>
constexpr bool holds_alternative(const variant<Types...>& v) noexcept {
  return v.index() == v_utils::find_pos_v<T, Types...>;
}

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars) {
  return visit<decltype(std::forward<Visitor>(vis)(get<0>(std::forward<Variants>(vars))...))>(
      std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }
  using fun = R (*)(Visitor&&, Variants && ...);
  return v_utils::visit_table<false, v_utils::multi_array<fun, variant_size_v<std::remove_cvref_t<Variants>>...>,
                              std::index_sequence<>>::stored.get_data(vars.index()...)(std::forward<Visitor>(vis),
                                                                                       std::forward<Variants>(vars)...);
}

namespace v_utils {

template <typename R, typename Visitor, typename... Variants>
constexpr R indexed_visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }
  using fun = R (*)(Visitor&&, Variants && ...);
  return v_utils::visit_table<true, v_utils::multi_array<fun, variant_size_v<std::remove_cvref_t<Variants>>...>,
                              std::index_sequence<>>::stored.get_data(vars.index()...)(std::forward<Visitor>(vis),
                                                                                       std::forward<Variants>(vars)...);
}

} // namespace v_utils

template <typename... Types>
constexpr bool operator==(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index()) {
    return false;
  }
  if (v.valueless_by_exception()) {
    return true;
  }
  return ::visit<bool>(
      []<typename A, typename B>(A&& a, B&& b) {
        if constexpr (std::is_same_v<A, B>) {
          return a == b;
        }
        return false;
      },
      v, w);
}

template <typename... Types>
constexpr bool operator!=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index()) {
    return true;
  }
  if (v.valueless_by_exception()) {
    return false;
  }
  return ::visit<bool>(
      []<typename A, typename B>(A&& a, B&& b) {
        if constexpr (std::is_same_v<A, B>) {
          return a != b;
        }
        return true;
      },
      v, w);
}

template <typename... Types>
constexpr bool operator<(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception()) {
    return false;
  }
  if (v.valueless_by_exception()) {
    return true;
  }
  if (v.index() != w.index()) {
    return v.index() < w.index();
  }
  return ::visit<bool>(
      []<typename A, typename B>(A&& a, B&& b) {
        if constexpr (std::is_same_v<A, B>) {
          return a < b;
        }
        return false;
      },
      v, w);
}

template <typename... Types>
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return false;
  }
  if (w.valueless_by_exception()) {
    return true;
  }
  if (v.index() != w.index()) {
    return v.index() > w.index();
  }
  return ::visit<bool>(
      []<typename A, typename B>(A&& a, B&& b) {
        if constexpr (std::is_same_v<A, B>) {
          return a > b;
        }
        return false;
      },
      v, w);
}

template <typename... Types>
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return true;
  }
  if (w.valueless_by_exception()) {
    return false;
  }
  if (v.index() != w.index()) {
    return v.index() < w.index();
  }
  return ::visit<bool>(
      []<typename A, typename B>(A&& a, B&& b) {
        if constexpr (std::is_same_v<A, B>) {
          return a <= b;
        }
        return false;
      },
      v, w);
}

template <typename... Types>
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception()) {
    return true;
  }
  if (v.valueless_by_exception()) {
    return false;
  }
  if (v.index() != w.index()) {
    return v.index() > w.index();
  }
  return ::visit<bool>(
      []<typename A, typename B>(A&& a, B&& b) {
        if constexpr (std::is_same_v<A, B>) {
          return a >= b;
        }
        return false;
      },
      v, w);
}
