#pragma once

#include "variant_utils.h"

namespace v_storage {

template <size_t I, typename Storage>
constexpr decltype(auto) get_impl(Storage&& s) {
  if constexpr (I == 0) {
    return (std::forward<Storage>(s).data);
  } else {
    return get_impl<I - 1>(std::forward<Storage>(s).rest);
  }
}

template <bool trivial_destructible, typename... Types>
struct union_storage {
  constexpr void reset(size_t) {}

  constexpr union_storage() noexcept = default;

  constexpr ~union_storage() = default;
};

template <typename Type, typename... Types>
struct union_storage<true, Type, Types...> {

  constexpr union_storage() noexcept : rest() {}

  template <size_t pos, typename... Args>
  constexpr explicit union_storage(in_place_index_t<pos>, Args&&... args)
      : rest(in_place_index<pos - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr explicit union_storage(in_place_index_t<0>, Args&&... args) {
    std::construct_at<Type>(std::addressof(data), std::forward<Args>(args)...);
  }

  constexpr ~union_storage() = default;

  constexpr void reset(size_t pos) {
    if (pos == 0) {
      data.~Type();
    } else {
      rest.reset(pos - 1);
    }
  }

  union {
    Type data;
    union_storage<true, Types...> rest;
  };
};

template <typename Type, typename... Types>
struct union_storage<false, Type, Types...> {

  constexpr union_storage() : rest() {}

  template <size_t pos, typename... Args>
  constexpr explicit union_storage(in_place_index_t<pos>, Args&&... args)
      : rest(in_place_index<pos - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr explicit union_storage(in_place_index_t<0>, Args&&... args) {
    std::construct_at<Type>(std::addressof(data), std::forward<Args>(args)...);
  }

  constexpr ~union_storage(){};

  constexpr void reset(size_t pos) {
    if (pos == 0) {
      data.~Type();
    } else {
      rest.reset(pos - 1);
    }
  }

  union {
    Type data;
    union_storage<false, Types...> rest;
  };
};

template <bool is_trivial, typename... Types>
struct define_destructor : union_storage<is_trivial, Types...> {
  using base = union_storage<is_trivial, Types...>;
  using union_storage<is_trivial, Types...>::union_storage;
  using base::reset;

  template <size_t pos, typename... Args>
  constexpr explicit define_destructor(in_place_index_t<pos> ind, Args&&... args)
      : base(ind, std::forward<Args>(args)...), ind{pos} {}

  constexpr ~define_destructor() = default;

  size_t ind{0};
};

template <typename... Types>
struct define_destructor<false, Types...> : union_storage<false, Types...> {
  using base = union_storage<false, Types...>;
  using union_storage<false, Types...>::union_storage;
  using base::reset;

  template <size_t pos, typename... Args>
  constexpr explicit define_destructor(in_place_index_t<pos> ind, Args&&... args)
      : base(ind, std::forward<Args>(args)...), ind{pos} {}

  constexpr ~define_destructor() {
    if (ind != variant_npos) {
      reset(ind);
    }
  }
  size_t ind{0};
};

template <typename... Types>
struct variant_storage : define_destructor<std::conjunction_v<std::is_trivially_destructible<Types>...>, Types...> {
  using define_destructor<std::conjunction_v<std::is_trivially_destructible<Types>...>, Types...>::define_destructor;
  using base = define_destructor<std::conjunction_v<std::is_trivially_destructible<Types>...>, Types...>;
  using base::ind;
  using base::reset;

  template <size_t pos, typename... Args>
  constexpr explicit variant_storage(in_place_index_t<pos> ind, Args&&... args)
      : base(ind, std::forward<Args>(args)...) {}

  constexpr void reset_storage() {
    if (ind != variant_npos) {
      reset(ind);
      ind = variant_npos;
    }
  }
};

} // namespace v_storage
