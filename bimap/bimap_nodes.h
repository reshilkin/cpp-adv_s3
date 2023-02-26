#ifndef BIMAP_NODES_H
#define BIMAP_NODES_H
#include <algorithm>
namespace nodes {

struct left_tag;
struct right_tag;

template <typename T>
constexpr static bool is_left = std::is_same_v<T, left_tag>;

struct tree_node;

template <typename Tag>
struct tag_node;

struct base_node;

template <typename L, typename R>
struct node;

namespace casts {
template <typename Tag>
base_node* tree_to_base(tree_node* ptr) {
  return static_cast<base_node*>(static_cast<tag_node<Tag>*>(ptr));
}

template <typename L, typename R>
node<L, R>* base_to_node(base_node* ptr) {
  return static_cast<node<L, R>*>(ptr);
}

template <typename L, typename R, typename Tag>
node<L, R>* tree_to_node(tree_node* ptr) {
  return base_to_node<L, R>(tree_to_base<Tag>(ptr));
}

template <typename L, typename R>
base_node* node_to_base(node<L, R>* ptr) {
  return static_cast<base_node*>(ptr);
}
template <typename Tag>
tree_node* base_to_tree(base_node* ptr) {
  return static_cast<tree_node*>(static_cast<tag_node<Tag>*>(ptr));
}

template <typename L, typename R, typename Tag>
tree_node* node_to_tree(node<L, R>* ptr) {
  return base_to_tree<Tag>(node_to_base(ptr));
}

} // namespace casts

struct tree_node {
  tree_node() = default;
  tree_node(tree_node&& other);

  tree_node& operator=(tree_node&& other);

  void construct_from_right_value(tree_node&& other);

  tree_node* maximum();
  tree_node* minimum();
  tree_node* next();
  tree_node* prev();

  void reparent(tree_node* ptr);
  tree_node* left_{nullptr};
  tree_node* right_{nullptr};
  tree_node* parent_{nullptr};
};

template <typename Tag>
struct tag_node : tree_node {};

struct base_node : tag_node<left_tag>, tag_node<right_tag> {};

template <typename L, typename R>
struct node : base_node {
  template <typename A, typename B>
  node(A&& left, B&& right)
      : l_element(std::forward<A>(left)), r_element(std::forward<B>(right)) {}
  L l_element;
  R r_element;
};
} // namespace nodes

#endif // BIMAP_NODES_H
