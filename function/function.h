#pragma once

#include <exception>

struct bad_function_call : std::exception {
  const char* what() const noexcept override {
    return "Calling empty function object";
  }
};

namespace details {

using storage_t = std::aligned_storage<sizeof(void*), alignof(void*)>;

template <typename T>
constexpr inline bool fits_small =
    sizeof(T) <= sizeof(storage_t) && alignof(storage_t) % alignof(T) == 0 &&
    std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T>;

template <typename T>
T const& get_ref(storage_t const& src) {
  if constexpr (fits_small<T>) {
    return *reinterpret_cast<T const*>(&src);
  } else {
    return **reinterpret_cast<T const* const*>(&src);
  }
}

template <typename T>
T& get_ref(storage_t& src) {
  if constexpr (fits_small<T>) {
    return *reinterpret_cast<T*>(&src);
  } else {
    return **reinterpret_cast<T**>(&src);
  }
}

template <typename R, typename... Args>
struct type_d {
  void (*const copy)(storage_t const& src, storage_t& dst);
  void (*const move)(storage_t& src, storage_t& dst);
  void (*const destroy)(storage_t& src);
  R (*const invoke)(storage_t const& src, Args... args);

  template <typename T>
  static type_d<R, Args...> const* get_descriptor() noexcept {
    constexpr static type_d<R, Args...> res = {
        [](storage_t const& src, storage_t& dst) {
          if constexpr (fits_small<T>) {
            new (&dst) T(get_ref<T>(src));
          } else {
            new (&dst) T*(new T(get_ref<T>(src)));
          }
        },
        [](storage_t& src, storage_t& dst) {
          if constexpr (fits_small<T>) {
            new (&dst) T(std::move(get_ref<T>(src)));
            get_ref<T>(src).~T();
          } else {
            new (&dst) T*(&get_ref<T>(src));
          }
        },
        [](storage_t& src) {
          if constexpr (fits_small<T>) {
            get_ref<T>(src).~T();
          } else {
            delete &get_ref<T>(src);
          }
        },
        [](storage_t const& src, Args... args) -> R {
          if constexpr (fits_small<T>) {
            return (get_ref<T>(src))(std::forward<Args>(args)...);
          } else {
            return (get_ref<T>(src))(std::forward<Args>(args)...);
          }
        }};
    return &res;
  }

  static type_d<R, Args...> const* get_empty_descriptor() noexcept {
    constexpr static type_d<R, Args...> res = {
        [](storage_t const& src, storage_t& dst) { dst = src; },
        [](storage_t& src, storage_t& dst) { dst = src; }, //
        [](storage_t&) {},
        [](storage_t const&, Args...) -> R { throw bad_function_call(); }};
    return &res;
  }
};
} // namespace details

template <typename F>
struct function;

template <typename R, typename... Args>
struct function<R(Args...)> {
  function() noexcept
      : desc(details::type_d<R, Args...>::get_empty_descriptor()) {}

  function(function const& other) : desc(other.desc) {
    other.desc->copy(other.storage, storage);
  }

  function(function&& other) noexcept : desc(other.desc) {
    other.desc->move(other.storage, storage);
    other.desc = details::type_d<R, Args...>::get_empty_descriptor();
  }

  template <typename T>
  function(T val)
      : desc(details::type_d<R, Args...>::template get_descriptor<T>()) {
    if constexpr (details::fits_small<T>) {
      new (&storage) T(std::move(val));
    } else {
      new (&storage) T*(new T(std::move(val)));
    }
  }

  function& operator=(function const& rhs) {
    if (&rhs != this) {
      function(rhs).swap(*this);
    }
    return *this;
  }
  function& operator=(function&& rhs) noexcept {
    if (&rhs != this) {
      desc->destroy(storage);
      rhs.desc->move(rhs.storage, storage);
      desc = rhs.desc;
      rhs.desc = details::type_d<R, Args...>::get_empty_descriptor();
    }
    return *this;
  }

  void swap(function& other) {
    function tmp = std::move(*this);
    *this = std::move(other);
    other = std::move(tmp);
  }

  ~function() {
    desc->destroy(storage);
  }

  explicit operator bool() const noexcept {
    return desc != details::type_d<R, Args...>::get_empty_descriptor();
  }

  R operator()(Args... args) const {
    return desc->invoke(storage, std::forward<Args>(args)...);
  }

  template <typename T>
  T* target() noexcept {
    if (details::type_d<R, Args...>::template get_descriptor<T>() != desc) {
      return nullptr;
    }
    if constexpr (details::fits_small<T>) {
      return reinterpret_cast<T*>(&storage);
    } else {
      return *reinterpret_cast<T**>(&storage);
    }
  }

  template <typename T>
  T const* target() const noexcept {
    if (details::type_d<R, Args...>::template get_descriptor<T>() != desc) {
      return nullptr;
    }
    if constexpr (details::fits_small<T>) {
      return reinterpret_cast<T const*>(&storage);
    } else {
      return *reinterpret_cast<T const* const*>(&storage);
    }
  }

private:
  details::type_d<R, Args...> const* desc;
  details::storage_t storage;
};
