#ifndef BIMAP_TREE_H
#define BIMAP_TREE_H
#include "bimap_nodes.h"

namespace bimap_tree {

template <typename T, typename Comparator, typename Getter>
struct tree : Comparator {
private:
  using tree_node_t = nodes::tree_node;

public:
  tree() = delete;

  tree(Comparator c, tree_node_t* fake)
      : Comparator(std::move(c)), fake_(fake) {}

  tree(tree const& other) = delete;

  // someone from the outside should create base_node_t from other's tree fake
  // base_node_t node and assign it's tree_node to fake_
  tree(tree&& other) = default;

  tree& operator=(tree const& other) = delete;

  tree& operator=(tree&& other) {
    if (&other == this) {
      return *this;
    }
    *static_cast<Comparator*>(this) =
        std::move(*static_cast<Comparator*>(&other));
    return *this;
  }

  void insert(tree_node_t* ptr) {
    tree_node_t* cur = fake_;
    while (cur != nullptr) {
      if (cur->parent_ == nullptr || compare(ptr, cur)) {
        if (cur->left_ != nullptr) {
          cur = cur->left_;
        } else {
          ptr->parent_ = cur;
          cur->left_ = ptr;
          break;
        }
      } else {
        if (cur->right_ != nullptr) {
          cur = cur->right_;
        } else {
          ptr->parent_ = cur;
          cur->right_ = ptr;
          break;
        }
      }
    }
  }

  void erase(tree_node_t* ptr) {
    tree_node_t* ret = ptr->next();
    if (ptr->left_ == nullptr && ptr->right_ == nullptr) {
      ptr->reparent(nullptr);
    } else if (ptr->left_ == nullptr) {
      ptr->reparent(ptr->right_);
    } else if (ptr->right_ == nullptr) {
      ptr->reparent(ptr->left_);
    } else {
      erase(ret);
      ptr->reparent(ret);
      if (ptr->left_ != nullptr) {
        ptr->left_->parent_ = ret;
        ret->left_ = ptr->left_;
      }
      if (ptr->right_ != nullptr) {
        ptr->right_->parent_ = ret;
        ret->right_ = ptr->right_;
      }
    }
    ptr->left_ = ptr->right_ = ptr->parent_ = nullptr;
  }

  tree_node_t* find(T const& elem) const {
    tree_node_t* cur = fake_;
    if (cur->left_ == nullptr) {
      return cur;
    }
    cur = cur->left_;
    while (true) {
      if (are_equal(get_elem(cur), elem)) {
        return cur;
      }
      if (cur->left_ != nullptr && compare(elem, get_elem(cur))) {
        cur = cur->left_;
      } else if (cur->right_ != nullptr && !compare(elem, get_elem(cur))) {
        cur = cur->right_;
      } else {
        return cur;
      }
    }
  }

  static T const& get_elem(tree_node_t* a) {
    return Getter::get(a);
  }

  bool compare(T const& a, T const& b) const {
    return Comparator::operator()(a, b);
  }

  bool are_equal(T const& a, T const& b) const {
    return !compare(a, b) && !compare(b, a);
  }

  bool compare(nodes::tree_node* a, nodes::tree_node* b) {
    return compare(get_elem(a), get_elem(b));
  }

  tree_node_t* fake_{nullptr};
};
} // namespace bimap_tree

#endif // BIMAP_TREE_H
