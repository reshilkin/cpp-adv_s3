#pragma once

#include "bimap_nodes.h"
#include "bimap_tree.h"
#include <cassert>
#include <cstddef>
#include <stdexcept>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
private:
  using left_t = Left;
  using right_t = Right;

  using tree_node_t = nodes::tree_node;

  using base_node_t = nodes::base_node;
  using node_t = nodes::node<left_t, right_t>;

  template <typename Tag>
  struct iterator {
    using value_type = std::conditional_t<nodes::is_left<Tag>, left_t, right_t>;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;

  private:
    using other_tag = std::conditional_t<nodes::is_left<Tag>, nodes::right_tag,
                                         nodes::left_tag>;
    explicit iterator(tree_node_t* node) : node_(node) {}

  public:
    auto const& operator*() const {
      auto ptr = nodes::casts::tree_to_node<left_t, right_t, Tag>(node_);
      if constexpr (nodes::is_left<Tag>) {
        return ptr->l_element;
      } else {
        return ptr->r_element;
      }
    }
    auto const* operator->() const {
      return &operator*();
    }

    iterator& operator++() {
      node_ = node_->next();
      return *this;
    }
    iterator operator++(int) {
      iterator res = *this;
      operator++();
      return res;
    }

    iterator& operator--() {
      node_ = node_->prev();
      return *this;
    }
    iterator operator--(int) {
      iterator res = *this;
      operator--();
      return res;
    }

    iterator<other_tag> flip() const {
      return iterator<other_tag>(
          nodes::casts::node_to_tree<left_t, right_t, other_tag>(
              nodes::casts::tree_to_node<left_t, right_t, Tag>(node_)));
    }

    friend bool operator==(iterator const& a, iterator const& b) {
      return a.node_ == b.node_;
    }

    friend bool operator!=(iterator const& a, iterator const& b) {
      return a.node_ != b.node_;
    }

    friend struct bimap;

  private:
    tree_node_t* node_;
  };

public:
  using left_iterator = iterator<nodes::left_tag>;
  using right_iterator = iterator<nodes::right_tag>;

  // Создает bimap не содержащий ни одной пары.
  explicit bimap(CompareLeft compare_left = CompareLeft(),
                 CompareRight compare_right = CompareRight())
      : left_tree_(std::move(compare_left), get_left(&fake_)),
        right_tree_(std::move(compare_right), get_right(&fake_)) {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other)
      : bimap(*static_cast<CompareLeft const*>(&other.left_tree_),
              *static_cast<CompareRight const*>(&other.right_tree_)) {
    left_tree_.fake_ = get_left(&fake_);
    right_tree_.fake_ = get_right(&fake_);
    try {
      for (left_iterator it = other.begin_left(); it != other.end_left();
           it++) {
        insert(*it, *(it.flip()));
      }
    } catch (...) {
      erase_left(begin_left(), end_left());
      throw;
    }
  }

  bimap(bimap&& other) noexcept
      : bimap(std::move(*static_cast<CompareLeft*>(&other.left_tree_)),
              std::move(*static_cast<CompareRight*>(&other.right_tree_))) {
    size_ = other.size_;
    fake_ = std::move(other.fake_);
    left_tree_.fake_ = get_left(&fake_);
    right_tree_.fake_ = get_right(&fake_);
    other.size_ = 0;
  }

  bimap& operator=(bimap const& other) {
    if (this == &other) {
      return *this;
    }
    bimap(other).swap(*this);
    return *this;
  }
  bimap& operator=(bimap&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    swap(other);
    return *this;
  }

  void swap(bimap& other) {
    std::swap(size_, other.size_);
    std::swap(left_tree_, other.left_tree_);
    std::swap(right_tree_, other.right_tree_);
    std::swap(fake_, other.fake_);
  }
  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  // Вставка пары (left_, right_), возвращает итератор на left_.
  // Если такой left_ или такой right_ уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_impl(left, right);
  }
  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }
  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_impl(std::move(left), right);
  }
  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    left_iterator res = std::next(it);
    erase_node(node_from_left(it.node_));
    delete node_from_left(it.node_);
    size_--;
    return res;
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    left_iterator it = find_left(left);
    if (it == end_left()) {
      return false;
    }
    erase_left(it);
    return true;
  }

  right_iterator erase_right(right_iterator it) {
    right_iterator res = std::next(it);
    erase_node(node_from_right(it.node_));
    delete node_from_right(it.node_);
    size_--;
    return res;
  }
  bool erase_right(right_t const& right) {
    right_iterator it = find_right(right);
    if (it == end_right()) {
      return false;
    }
    erase_right(it);
    return true;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    while (first != last) {
      first = erase_left(first);
    }
    return first;
  }
  right_iterator erase_right(right_iterator first, right_iterator last) {
    while (first != last) {
      first = erase_right(first);
    }
    return first;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    tree_node_t* ptr = left_tree_.find(left);
    if (ptr == left_tree_.fake_ ||
        !eq_left(node_from_left(ptr)->l_element, left)) {
      return end_left();
    }
    return left_iterator(ptr);
  }
  right_iterator find_right(right_t const& right) const {
    tree_node_t* ptr = right_tree_.find(right);
    if (ptr == right_tree_.fake_ ||
        !eq_right(node_from_right(ptr)->r_element, right)) {
      return end_right();
    }
    return right_iterator(ptr);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    left_iterator it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("No such key");
    }
    return *(it.flip());
  }
  left_t const& at_right(right_t const& key) const {
    right_iterator it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("No such key");
    }
    return *(it.flip());
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename R = right_t,
            typename std::enable_if_t<std::is_default_constructible_v<R>,
                                      bool> = true>
  right_t const& at_left_or_default(left_t const& key) {
    left_iterator it_l = find_left(key);
    if (it_l != end_left()) {
      return *(it_l.flip());
    }
    right_t tmp = right_t();
    right_iterator it_r = find_right(tmp);
    if (it_r == end_right()) {
      return *(insert(key, std::move(tmp)).flip());
    } else {
      erase_node(node_from_right(it_r.node_));
      node_from_right(it_r.node_)->l_element = key;
      insert_node(node_from_right(it_r.node_));
      return *it_r;
    }
  }

  template <typename L = left_t,
            typename std::enable_if_t<std::is_default_constructible_v<L>,
                                      bool> = true>
  left_t const& at_right_or_default(right_t const& key) {
    right_iterator it_r = find_right(key);
    if (it_r != end_right()) {
      return *(it_r.flip());
    }
    left_t tmp = left_t();
    left_iterator it_l = find_left(tmp);
    if (it_l == end_left()) {
      return *insert(std::move(tmp), key);
    } else {
      erase_node(node_from_left(it_l.node_));
      node_from_left(it_l.node_)->r_element = key;
      insert_node(node_from_left(it_l.node_));
      return *it_l;
    }
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    tree_node_t* ptr = left_tree_.find(left);
    left_iterator it(ptr);
    if (it == end_left() || eq_left(*it, left) || cmp_left(left, *it)) {
      return it;
    }
    return ++it;
  }
  left_iterator upper_bound_left(const left_t& left) const {
    tree_node_t* ptr = left_tree_.find(left);
    left_iterator it(ptr);
    if (it == end_left() || cmp_left(left, *it)) {
      return it;
    }
    return ++it;
  }

  right_iterator lower_bound_right(const right_t& right) const {
    tree_node_t* ptr = right_tree_.find(right);
    right_iterator it(ptr);
    if (it == end_right() || eq_right(*it, right) || cmp_right(right, *it)) {
      return it;
    }
    return ++it;
  }
  right_iterator upper_bound_right(const right_t& right) const {
    tree_node_t* ptr = right_tree_.find(right);
    right_iterator it(ptr);
    if (it == end_right() || cmp_right(right, *it)) {
      return it;
    }
    return ++it;
  }

  // Возващает итератор на минимальный по порядку left_.
  left_iterator begin_left() const {
    return left_iterator(left_tree_.fake_->minimum());
  }
  // Возващает итератор на следующий за последним по порядку left_.
  left_iterator end_left() const {
    return left_iterator(left_tree_.fake_);
  }

  // Возващает итератор на минимальный по порядку right_.
  right_iterator begin_right() const {
    return right_iterator(right_tree_.fake_->minimum());
  }
  // Возващает итератор на следующий за последним по порядку right_.
  right_iterator end_right() const {
    return right_iterator(right_tree_.fake_);
  }

  // Проверка на пустоту
  bool empty() const {
    return size_ == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return size_;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    if (a.size() != b.size()) {
      return false;
    }
    for (left_iterator it1 = a.begin_left(), it2 = b.begin_left();
         it1 != a.end_left(); it1++, it2++) {
      if (!a.eq_left(*it1, *it2) || !a.eq_right(*(it1.flip()), *(it2.flip()))) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(bimap const& a, bimap const& b) {
    return !(a == b);
  }

private:
  template <typename A, typename B>
  left_iterator insert_impl(A&& left, B&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    auto* node = new node_t(std::forward<A>(left), std::forward<B>(right));
    insert_node(node);
    size_++;
    return left_iterator(get_left(node));
  }

  void erase_node(node_t* node) {
    left_tree_.erase(get_left(node));
    right_tree_.erase(get_right(node));
  }
  void insert_node(node_t* node) {
    left_tree_.insert(get_left(node));
    right_tree_.insert(get_right(node));
  }

  bool eq_left(left_t const& a, left_t const& b) const {
    return left_tree_.are_equal(a, b);
  }

  bool eq_right(right_t const& a, right_t const& b) const {
    return right_tree_.are_equal(a, b);
  }

  bool cmp_left(left_t const& a, left_t const& b) const {
    return left_tree_.compare(a, b);
  }

  bool cmp_right(right_t const& a, right_t const& b) const {
    return right_tree_.compare(a, b);
  }

  static tree_node_t* get_left(base_node_t* ptr) {
    return nodes::casts::base_to_tree<nodes::left_tag>(ptr);
  }
  static tree_node_t* get_left(node_t* ptr) {
    return get_left(nodes::casts::node_to_base(ptr));
  }

  static tree_node_t* get_right(base_node_t* ptr) {
    return nodes::casts::base_to_tree<nodes::right_tag>(ptr);
  }
  static tree_node_t* get_right(node_t* ptr) {
    return get_right(nodes::casts::node_to_base(ptr));
  }

  template <typename Tag>
  static node_t* get_node(tree_node_t* ptr) {
    return nodes::casts::tree_to_node<left_t, right_t, Tag>(ptr);
  }

  static node_t* node_from_left(tree_node_t* ptr) {
    return get_node<nodes::left_tag>(ptr);
  }
  static node_t* node_from_right(tree_node_t* ptr) {
    return get_node<nodes::right_tag>(ptr);
  }

  struct left_getter {
    static left_t const& get(tree_node_t* ptr) {
      return node_from_left(ptr)->l_element;
    }
  };

  struct right_getter {
    static right_t const& get(tree_node_t* ptr) {
      return node_from_right(ptr)->r_element;
    }
  };

  bimap_tree::tree<left_t, CompareLeft, left_getter> left_tree_;
  bimap_tree::tree<right_t, CompareRight, right_getter> right_tree_;
  base_node_t fake_;
  size_t size_{0};
};
