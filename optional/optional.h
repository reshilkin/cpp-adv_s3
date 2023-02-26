#pragma once
#include "enable-bases.h"
#include <type_traits>
#include <utility>

struct nullopt_t {};
inline const nullopt_t nullopt;

struct in_place_t {};
inline const in_place_t in_place;

namespace trivial {

template <typename T, bool trivial = std::is_trivially_destructible_v<T>>
struct trivial_destruct_base {
  constexpr trivial_destruct_base() noexcept : dummy_{0} {}
  constexpr trivial_destruct_base(trivial_destruct_base const&) = default;
  constexpr trivial_destruct_base(trivial_destruct_base&&) = default;
  constexpr trivial_destruct_base&
  operator=(trivial_destruct_base const&) = default;
  constexpr trivial_destruct_base& operator=(trivial_destruct_base&&) = default;

  template <typename... Args>
  constexpr trivial_destruct_base(in_place_t, Args&&... args)
      : is_initialized_(true), data_(std::forward<Args>(args)...) {}

  constexpr trivial_destruct_base(nullopt_t) noexcept
      : trivial_destruct_base() {}

  void reset_impl() {
    if (is_initialized_) {
      data_.~T();
      is_initialized_ = false;
    }
  }

  ~trivial_destruct_base() {
    reset_impl();
  }

  bool is_initialized_{false};
  union {
    char dummy_{0};
    T data_;
  };
};

template <typename T>
struct trivial_destruct_base<T, true> {
  constexpr trivial_destruct_base() noexcept : dummy_{0} {}
  constexpr trivial_destruct_base(trivial_destruct_base const&) = default;
  constexpr trivial_destruct_base(trivial_destruct_base&&) = default;
  constexpr trivial_destruct_base&
  operator=(trivial_destruct_base const&) = default;
  constexpr trivial_destruct_base& operator=(trivial_destruct_base&&) = default;

  template <typename... Args>
  constexpr trivial_destruct_base(in_place_t, Args&&... args)
      : is_initialized_(true), data_(std::forward<Args>(args)...) {}

  constexpr trivial_destruct_base(nullopt_t) noexcept
      : trivial_destruct_base() {}

  void reset_impl() {
    is_initialized_ = false;
  }

  bool is_initialized_{false};
  union {
    char dummy_{0};
    T data_;
  };
};

template <typename T, bool trivial = std::is_trivially_copy_assignable_v<T>&&
                          std::is_trivially_copy_constructible_v<T>>
struct trivial_copy_assign_base : trivial_destruct_base<T> {
  using trivial_destruct_base<T>::trivial_destruct_base;
  constexpr trivial_copy_assign_base() = default;
  constexpr trivial_copy_assign_base(trivial_copy_assign_base const&) = default;
  constexpr trivial_copy_assign_base(trivial_copy_assign_base&&) = default;
  constexpr trivial_copy_assign_base&
  operator=(trivial_copy_assign_base const& other) {
    if (this == &other) {
      return *this;
    }
    if (this->is_initialized_) {
      if (other.is_initialized_) {
        this->data_ = other.data_;
      } else {
        this->reset_impl();
      }
    } else {
      if (other.is_initialized_) {
        this->is_initialized_ = true;
        new (&(this->data_)) T(other.data_);
      }
    }
    return *this;
  }
  constexpr trivial_copy_assign_base&
  operator=(trivial_copy_assign_base&&) = default;
};

template <typename T>
struct trivial_copy_assign_base<T, true> : trivial_destruct_base<T> {
  using trivial_destruct_base<T>::trivial_destruct_base;
};

template <typename T, bool trivial = std::is_trivially_move_assignable_v<T>&&
                          std::is_trivially_move_constructible_v<T>>
struct trivial_move_assign_base : trivial_copy_assign_base<T> {
  using trivial_copy_assign_base<T>::trivial_copy_assign_base;
  constexpr trivial_move_assign_base() = default;
  constexpr trivial_move_assign_base(trivial_move_assign_base const&) = default;
  constexpr trivial_move_assign_base(trivial_move_assign_base&&) = default;
  constexpr trivial_move_assign_base&
  operator=(trivial_move_assign_base const&) = default;
  constexpr trivial_move_assign_base&
  operator=(trivial_move_assign_base&& other) {
    if (this == &other) {
      return *this;
    }
    if (this->is_initialized_) {
      if (other.is_initialized_) {
        this->data_ = std::move(other.data_);
      } else {
        this->reset_impl();
      }
    } else {
      if (other.is_initialized_) {
        this->is_initialized_ = true;
        new (&(this->data_)) T(std::move(other.data_));
      }
    }
    return *this;
  }
};

template <typename T>
struct trivial_move_assign_base<T, true> : trivial_copy_assign_base<T> {
  using trivial_copy_assign_base<T>::trivial_copy_assign_base;
};

template <typename T, bool trivial = std::is_trivially_copy_constructible_v<T>>
struct trivial_copy_construct_base : trivial_move_assign_base<T> {
  using trivial_move_assign_base<T>::trivial_move_assign_base;
  constexpr trivial_copy_construct_base() = default;
  constexpr trivial_copy_construct_base(
      trivial_copy_construct_base const& other) {
    if (other.is_initialized_) {
      this->is_initialized_ = true;
      new (&(this->data_)) T(other.data_);
    } else {
      this->is_initialized_ = false;
    }
  }
  constexpr trivial_copy_construct_base(trivial_copy_construct_base&&) =
      default;
  constexpr trivial_copy_construct_base&
  operator=(trivial_copy_construct_base const&) = default;
  constexpr trivial_copy_construct_base&
  operator=(trivial_copy_construct_base&&) = default;
};

template <typename T>
struct trivial_copy_construct_base<T, true> : trivial_move_assign_base<T> {
  using trivial_move_assign_base<T>::trivial_move_assign_base;
};

template <typename T, bool trivial = std::is_trivially_move_constructible_v<T>>
struct trivial_move_construct_base : trivial_copy_construct_base<T> {
  using trivial_copy_construct_base<T>::trivial_copy_construct_base;
  constexpr trivial_move_construct_base() = default;
  constexpr trivial_move_construct_base(trivial_move_construct_base const&) =
      default;
  constexpr trivial_move_construct_base(trivial_move_construct_base&& other) {
    if (other.is_initialized_) {
      this->is_initialized_ = true;
      new (&(this->data_)) T(std::move(other.data_));
    } else {
      this->is_initialized_ = false;
    }
  }
  constexpr trivial_move_construct_base&
  operator=(trivial_move_construct_base const&) = default;
  constexpr trivial_move_construct_base&
  operator=(trivial_move_construct_base&&) = default;
};
template <typename T>
struct trivial_move_construct_base<T, true> : trivial_copy_construct_base<T> {
  using trivial_copy_construct_base<T>::trivial_copy_construct_base;
};
} // namespace trivial

template <typename T>
class optional : trivial::trivial_move_construct_base<T>,
                 enable::move_assign_base<T>,
                 enable::copy_assign_base<T>,
                 enable::move_construct_base<T>,
                 enable::copy_construct_base<T> {
  using trivial::trivial_move_construct_base<T>::trivial_move_construct_base;

public:
  constexpr explicit operator bool() const noexcept {
    return this->is_initialized_;
  }

  constexpr optional& operator=(nullopt_t) noexcept {
    this->reset_impl();
    return *this;
  }

  constexpr optional(T x) : optional(in_place, std::move(x)) {}

  constexpr T& operator*() noexcept {
    return this->data_;
  }
  constexpr T const& operator*() const noexcept {
    return this->data_;
  }

  constexpr T* operator->() noexcept {
    return &(this->data_);
  }
  constexpr T const* operator->() const noexcept {
    return &(this->data_);
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    this->reset_impl();
    new (&(this->data_)) T(std::forward<Args>(args)...);
    this->is_initialized_ = true;
  }

  void swap(optional& other) {
    if (*this && other) {
      using std::swap;
      swap(this->data, other.data);
    } else if (*this && !other) {
      other = std::move(*this);
      this->reset_impl();
    } else if (!*this && other) {
      other.swap(*this);
    }
  }

  void reset() {
    this->reset_impl();
  }
};

template <typename T>
constexpr bool operator==(optional<T> const& a, optional<T> const& b) {
  return (!a && !b) || (a && b && *a == *b);
}

template <typename T>
constexpr bool operator!=(optional<T> const& a, optional<T> const& b) {
  return !(a == b);
}

template <typename T>
constexpr bool operator<(optional<T> const& a, optional<T> const& b) {
  return (!a && b) || (a && b && *a < *b);
}

template <typename T>
constexpr bool operator<=(optional<T> const& a, optional<T> const& b) {
  return (a < b) || (a == b);
}

template <typename T>
constexpr bool operator>(optional<T> const& a, optional<T> const& b) {
  return !(a <= b);
}

template <typename T>
constexpr bool operator>=(optional<T> const& a, optional<T> const& b) {
  return !(a < b);
}
