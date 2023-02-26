#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

template <typename T>
struct shared_ptr;

template <typename T>
struct weak_ptr;

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&...);

namespace details {
struct control_block {
  virtual void unlink() = 0;
  void strong_inc();
  void weak_inc();
  void strong_dec();
  void weak_dec();
  virtual ~control_block() = default;
  template <typename S>
  friend struct ::shared_ptr;

  template <typename S>
  friend struct ::weak_ptr;

  template <typename S, typename D>
  friend struct ptr_block;

  template <typename S>
  friend struct object_block;

private:
  size_t strong_ref_{1};
  size_t weak_ref_{1};
};

template <typename T>
struct default_deleter {
  void operator()(T* object) {
    delete object;
  }
};

template <typename T, typename D = default_deleter<T>>
struct ptr_block : control_block {
  explicit ptr_block<T, D>(std::nullptr_t) : ptr_(nullptr), deleter_() {}
  explicit ptr_block(T* ptr) : ptr_(ptr), deleter_() {}
  ptr_block(T* ptr, D&& deleter) : ptr_(ptr), deleter_(std::move(deleter)) {}
  ~ptr_block() override = default;

private:
  void unlink() override {
    if (ptr_) {
      deleter_(ptr_);
      ptr_ = nullptr;
    }
  }
  T* ptr_{nullptr};
  D deleter_;
};

template <typename T>
struct object_block : public control_block {
  template <typename... Args>
  explicit object_block(Args&&... args) {
    new (&object_) T(std::forward<Args...>(args)...);
  }
  ~object_block() override = default;
  template <typename S, typename... Args>
  shared_ptr<S> friend ::make_shared(Args&&...);

private:
  T* get_ptr() {
    return reinterpret_cast<T*>(&object_);
  }
  std::aligned_storage_t<sizeof(T), alignof(T)> object_;
  void unlink() override {
    get_ptr()->~T();
  }
};
} // namespace details

template <typename T>
struct shared_ptr {
public:
  shared_ptr() noexcept = default;
  shared_ptr(std::nullptr_t) noexcept : shared_ptr() {}
  shared_ptr(T* ptr) : ptr_(ptr) {
    try {
      cb_ = new details::ptr_block(ptr);
    } catch (...) {
      delete ptr;
      throw;
    }
  }
  template <typename U, std::enable_if_t<std::is_base_of_v<T, U> &&
                                             !std::is_same_v<const U, const T>,
                                         bool> = true>
  shared_ptr(U* ptr) : ptr_(ptr) {
    try {
      cb_ = new details::ptr_block(ptr);
    } catch (...) {
      delete ptr;
      throw;
    }
  }

private:
  shared_ptr(T* ptr, details::control_block* cb) : ptr_(ptr), cb_(cb) {
    if (cb_) {
      cb_->strong_inc();
    }
  }

public:
  template <typename P>
  shared_ptr(shared_ptr<P> const& other, T* ptr) noexcept
      : shared_ptr(ptr, other.cb_) {}

  template <typename D>
  shared_ptr(T* ptr, D&& deleter) : ptr_(ptr) {
    try {
      cb_ = new details::ptr_block<T, D>(ptr, std::move(deleter));
    } catch (...) {
      deleter(ptr);
      throw;
    }
  }
  shared_ptr(shared_ptr const& other) noexcept
      : shared_ptr(other.ptr_, other.cb_) {}

  template <typename U,
            std::enable_if_t<std::is_same_v<T, const U>, bool> = true>
  shared_ptr(shared_ptr<U> const& other) noexcept
      : shared_ptr(const_cast<U const*>(other.ptr_), other.cb_) {}

  template <typename U, std::enable_if_t<std::is_base_of_v<T, U> &&
                                             !std::is_same_v<const U, const T>,
                                         bool> = true>
  shared_ptr(shared_ptr<U> const& other) noexcept
      : shared_ptr(other, static_cast<T*>(other.ptr_)) {}

  shared_ptr(shared_ptr&& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
    other.ptr_ = nullptr;
    other.cb_ = nullptr;
  }
  ~shared_ptr() {
    if (cb_) {
      cb_->strong_dec();
    }
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    if (&other == this) {
      return *this;
    }
    shared_ptr(other).swap(*this);
    return *this;
  }
  shared_ptr& operator=(shared_ptr&& other) noexcept {
    if (&other == this) {
      return *this;
    }
    reset();
    swap(other);
    return *this;
  }
  template <typename U,
            std::enable_if_t<std::is_same_v<T, const U>, bool> = true>
  shared_ptr& operator=(shared_ptr<U> const& other) noexcept {
    shared_ptr<T>(other).swap(*this);
    return *this;
  }

  T* get() const noexcept {
    return ptr_;
  }
  explicit operator bool() const noexcept {
    return ptr_;
  }
  T& operator*() const noexcept {
    return *ptr_;
  }
  T* operator->() const noexcept {
    return ptr_;
  }
  friend bool operator==(shared_ptr const& a, shared_ptr const& b) {
    return a.ptr_ == b.ptr_;
  }
  friend bool operator!=(shared_ptr const& a, shared_ptr const& b) {
    return a.ptr_ != b.ptr_;
  }

  std::size_t use_count() const noexcept {
    return cb_ ? cb_->strong_ref_ : 0;
  }
  void reset() noexcept {
    if (cb_) {
      cb_->strong_dec();
      cb_ = nullptr;
      ptr_ = nullptr;
    }
  }
  void reset(T* new_ptr) {
    operator=(new_ptr);
  }
  template <typename U, std::enable_if_t<std::is_base_of_v<T, U> &&
                                             !std::is_same_v<const U, const T>,
                                         bool> = true>
  void reset(U* new_ptr) {
    operator=(new_ptr);
  }
  template <typename D>
  void reset(T* new_ptr, D&& deleter) {
    operator=(shared_ptr(new_ptr, std::move(deleter)));
  }
  void swap(shared_ptr& other) {
    std::swap(cb_, other.cb_);
    std::swap(ptr_, other.ptr_);
  }

  template <typename S>
  friend struct weak_ptr;

  template <typename S>
  friend struct shared_ptr;

  template <typename S, typename... Args>
  shared_ptr<S> friend ::make_shared(Args&&...);

private:
  T* ptr_{nullptr};
  details::control_block* cb_{nullptr};
};

template <typename T>
struct weak_ptr {
  weak_ptr() noexcept = default;
  weak_ptr(shared_ptr<T> const& other) noexcept
      : ptr_(other.ptr_), cb_(other.cb_) {
    if (cb_) {
      cb_->weak_inc();
    }
  }
  weak_ptr(weak_ptr const& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
    if (cb_) {
      cb_->weak_inc();
    }
  }
  weak_ptr(weak_ptr&& other) noexcept {
    reset();
    swap(other);
  }
  ~weak_ptr() noexcept {
    if (cb_) {
      cb_->weak_dec();
    }
  }
  weak_ptr<T>& operator=(shared_ptr<T> const& other) noexcept {
    weak_ptr(other).swap(*this);
    return *this;
  }
  weak_ptr& operator=(weak_ptr const& other) noexcept {
    if (&other == this) {
      return *this;
    }
    weak_ptr(other).swap(*this);
    return *this;
  }
  weak_ptr& operator=(weak_ptr&& other) noexcept {
    if (&other == this) {
      return *this;
    }
    reset();
    swap(other);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
    if (cb_ && cb_->strong_ref_ == 0) {
      return shared_ptr<T>();
    } else {
      return shared_ptr<T>(ptr_, cb_);
    }
  }

  void swap(weak_ptr& other) {
    std::swap(cb_, other.cb_);
    std::swap(ptr_, other.ptr_);
  }

  void reset() {
    if (cb_) {
      cb_->weak_dec();
      cb_ = nullptr;
      ptr_ = nullptr;
    }
  }

private:
  T* ptr_{nullptr};
  details::control_block* cb_{nullptr};
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* ob = new details::object_block<T>(std::forward<Args...>(args)...);
  shared_ptr<T> res(ob->get_ptr(), static_cast<details::control_block*>(ob));
  ob->strong_dec();
  return res;
}
