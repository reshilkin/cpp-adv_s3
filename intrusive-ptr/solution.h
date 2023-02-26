template <typename T>
struct intrusive_ptr {
  using element_type = T;

  intrusive_ptr() noexcept = default;
  intrusive_ptr(T* p, bool add_ref = true) : ptr_(p) {
    if (p != nullptr && add_ref) {
      intrusive_ptr_add_ref(ptr_);
    }
  }

  intrusive_ptr(intrusive_ptr const& r) : ptr_(r.get()) {
    if (r) {
      intrusive_ptr_add_ref(ptr_);
    }
  }
  template <class Y>
  requires(std::is_convertible_v<std::add_pointer_t<Y>, std::add_pointer_t<T>>)
      intrusive_ptr(intrusive_ptr<Y> const& r)
      : ptr_(static_cast<T*>(r.get())) {
    if (r) {
      intrusive_ptr_add_ref(ptr_);
    }
  }

  intrusive_ptr(intrusive_ptr&& r) : ptr_(r.get()) {
    r.ptr_ = nullptr;
  }

  template <class Y>
  friend struct intrusive_ptr;

  template <class Y>
  requires(std::is_convertible_v<std::add_pointer_t<Y>, std::add_pointer_t<T>>)
      intrusive_ptr(intrusive_ptr<Y>&& r)
      : ptr_(static_cast<T*>(r.get())) {
    r.ptr_ = nullptr;
  }

  ~intrusive_ptr() {
    if (ptr_ != nullptr) {
      intrusive_ptr_release(ptr_);
    }
  }

  intrusive_ptr& operator=(intrusive_ptr const& r) {
    intrusive_ptr(r).swap(*this);
    return *this;
  }
  template <class Y>
  intrusive_ptr& operator=(intrusive_ptr<Y> const& r) {
    intrusive_ptr(r).swap(*this);
    return *this;
  }
  intrusive_ptr& operator=(T* r) {
    intrusive_ptr(r).swap(*this);
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& r) {
    intrusive_ptr(std::move(r)).swap(*this);
    return *this;
  }
  template <class Y>
  intrusive_ptr& operator=(intrusive_ptr<Y>&& r) {
    intrusive_ptr(std::move(r)).swap(*this);
    return *this;
  }

  void reset() {
    intrusive_ptr().swap(*this);
  }
  void reset(T* r) {
    intrusive_ptr(r).swap(*this);
  }
  void reset(T* r, bool add_ref) {
    intrusive_ptr(r, add_ref).swap(*this);
  }

  T& operator*() const noexcept {
    return *ptr_;
  }
  T* operator->() const noexcept {
    return ptr_;
  }
  T* get() const noexcept {
    return ptr_;
  }
  T* detach() noexcept {
    T* res = ptr_;
    ptr_ = nullptr;
    return res;
  }

  explicit operator bool() const noexcept {
    return ptr_ != nullptr;
  }

  void swap(intrusive_ptr& b) noexcept {
    using std::swap;
    swap(b.ptr_, ptr_);
  }

private:
  T* ptr_{nullptr};
};

template <class T, class U>
bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept {
  return a.get() == b.get();
}

template <class T, class U>
bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept {
  return a.get() != b.get();
}

template <class T, class U>
bool operator==(intrusive_ptr<T> const& a, U* b) noexcept {
  return a.get() == b;
}

template <class T, class U>
bool operator!=(intrusive_ptr<T> const& a, U* b) noexcept {
  return a.get() != b;
}

template <class T, class U>
bool operator==(T* a, intrusive_ptr<U> const& b) noexcept {
  return a == b.get();
}

template <class T, class U>
bool operator!=(T* a, intrusive_ptr<U> const& b) noexcept {
  return a != b.get();
}

template <class T>
bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<T> const& b) noexcept {
  return std::less<T*>()(a.get(), b.get());
}

template <class T>
void swap(intrusive_ptr<T>& a, intrusive_ptr<T>& b) noexcept {
  a.swap(b);
}

template <typename Derived>
struct intrusive_ref_counter {
  intrusive_ref_counter() noexcept = default;
  intrusive_ref_counter(const intrusive_ref_counter& v) noexcept = default;

  intrusive_ref_counter&
  operator=(const intrusive_ref_counter& v) noexcept = default;

  unsigned int use_count() const noexcept {
    return count_.load(std::memory_order_acquire);
  }

  template <class T>
  friend void intrusive_ptr_add_ref(const intrusive_ref_counter<T>*) noexcept;

  template <class T>
  friend void intrusive_ptr_release(const intrusive_ref_counter<T>*) noexcept;

protected:
  ~intrusive_ref_counter() = default;

private:
  mutable std::atomic<unsigned int> count_{0};
};

template <class Derived>
void intrusive_ptr_add_ref(const intrusive_ref_counter<Derived>* p) noexcept {
  p->count_.fetch_add(1, std::memory_order_acq_rel);
}

template <class Derived>
void intrusive_ptr_release(const intrusive_ref_counter<Derived>* p) noexcept {
  if (p->count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    delete static_cast<const Derived*>(p);
  }
}
