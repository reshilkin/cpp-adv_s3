#pragma once

#include <cstddef>
#include <iterator>
namespace intrusive {
struct default_tag;

struct list_element_base {

  list_element_base();

  list_element_base(const list_element_base&);

  list_element_base(list_element_base&&);

  list_element_base& operator=(list_element_base&&);

  void unlink();

  void link_to_yourself();

  void link_forward(list_element_base*);

  ~list_element_base();

  template <typename T, typename Tag>
  friend struct list;

private:
  list_element_base* prev;
  list_element_base* next;
};

template <typename Tag = default_tag>
struct list_element : list_element_base {
  list_element() = default;
};

template <typename T, typename Tag = default_tag>
struct list {
public:
  template <typename S>
  struct list_iterator {

    using value_type = S;
    using reference = S&;
    using pointer = S*;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;

    list_iterator(S* ptr) : ptr_(static_cast<list_element_base*>(static_cast<list_element<Tag>*>(ptr))) {}

    list_iterator(S* ptr) requires(std::is_const_v<S>)
        : ptr_(const_cast<list_element_base*>(
              static_cast<list_element_base const*>(static_cast<list_element<Tag> const*>(ptr)))) {}

  private:
    list_iterator(list_element_base* ptr) : ptr_(ptr) {} // friend + list

  public:
    list_iterator() = default;
    ~list_iterator() = default;

    list_iterator(list_iterator const& other) : ptr_(other.ptr_) {}

    template <typename A, std::enable_if_t<std::is_same<S, const A>::value, bool> = true>
    list_iterator(list_iterator<A> const& other) : ptr_(other.ptr_) {}

    reference operator*() {
      return *(static_cast<S*>(static_cast<list_element<Tag>*>(ptr_)));
    }

    reference operator*() const {
      return *(static_cast<S*>(static_cast<list_element<Tag>*>(ptr_)));
    }

    pointer operator->() {
      return static_cast<S*>(static_cast<list_element<Tag>*>(ptr_));
    }

    pointer operator->() const {
      return static_cast<S*>(static_cast<list_element<Tag>*>(ptr_));
    }

    list_iterator& operator++() {
      ptr_ = ptr_->next;
      return *this;
    }

    list_iterator operator++(int) {
      list_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    list_iterator& operator--() {
      ptr_ = ptr_->prev;
      return *this;
    }

    list_iterator operator--(int) {
      list_iterator tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(list_iterator const& a, list_iterator const& b) {
      return a.ptr_ == b.ptr_;
    }

    friend bool operator!=(list_iterator const& a, list_iterator const& b) {
      return a.ptr_ != b.ptr_;
    }
    friend list;

  private:
    list_element_base* ptr_{nullptr};
  };

  using iterator = list_iterator<T>;

  using const_iterator = list_iterator<const T>;

  list() = default;

  list(list&& other) = default;

  list& operator=(list&& other) {
    iterator it = begin();
    while (it != end()) {
      it = erase(it);
    }
    fake = std::move(other.fake);
    return *this;
  }

  iterator begin() {
    return fake.next;
  }

  const_iterator begin() const {
    return fake.next;
  }

  iterator end() {
    return &fake;
  }

  const_iterator end() const {
    return const_cast<list_element_base*>(&fake);
  }

  void push_front(T& e) {
    insert(begin(), e);
  }

  void push_back(T& e) {
    insert(end(), e);
  }

  T& front() {
    return *begin();
  }

  T const& front() const {
    return *begin();
  }

  T& back() {
    return *(--end());
  }

  T const& back() const {
    return *(--end());
  }

  bool empty() {
    return fake.next == &fake;
  }

  void pop_front() {
    erase(begin());
  }

  void pop_back() {
    erase(--end());
  }

  iterator insert(iterator pos, T& e) {
    if (pos != &e) {
      auto& cur = static_cast<list_element_base&>(static_cast<list_element<Tag>&>(e));
      cur.unlink();
      link(pos.ptr_->prev, &cur);
      link(&cur, pos.ptr_);
      pos = iterator(&cur);
    }
    return pos;
  }

  iterator erase(iterator pos) {
    iterator res = pos;
    res++;
    pos.ptr_->unlink();
    return res;
  }

  void splice(const_iterator pos, list& other, const_iterator beg, const_iterator end) {
    if (beg == end) {
      return;
    }
    iterator first = end->prev;
    link(beg.ptr_->prev, end.ptr_);
    link(pos.ptr_->prev, beg.ptr_);
    link(first.ptr_, pos.ptr_);
  }

private:
  void link(list_element_base* a, list_element_base* b) {
    a->link_forward(b);
  }
  list_element_base fake;
};
} // namespace intrusive
