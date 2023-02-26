#include "bimap_nodes.h"

nodes::tree_node::tree_node(tree_node&& other) {
  construct_from_right_value(std::move(other));
}
nodes::tree_node& nodes::tree_node::operator=(nodes::tree_node&& other) {
  if (&other == this) {
    return *this;
  }
  construct_from_right_value(std::move(other));
  return *this;
}
void nodes::tree_node::construct_from_right_value(nodes::tree_node&& other) {
  left_ = other.left_;
  right_ = other.right_;
  other.reparent(this);
  if (left_ != nullptr) {
    left_->parent_ = this;
  }
  if (right_ != nullptr) {
    right_->parent_ = this;
  }
  other.left_ = other.right_ = other.parent_ = nullptr;
}
nodes::tree_node* nodes::tree_node::maximum() {
  if (right_ == nullptr) {
    return this;
  }
  return right_->maximum();
}
nodes::tree_node* nodes::tree_node::minimum() {
  if (left_ == nullptr) {
    return this;
  }
  return left_->minimum();
}
nodes::tree_node* nodes::tree_node::next() {
  if (right_ != nullptr) {
    return right_->minimum();
  }
  tree_node* a = this;
  tree_node* b = parent_;
  while (b != nullptr && a == b->right_) {
    a = b;
    b = b->parent_;
  }
  return b;
}
nodes::tree_node* nodes::tree_node::prev() {
  if (left_ != nullptr) {
    return left_->maximum();
  }
  tree_node* a = this;
  tree_node* b = parent_;
  while (b != nullptr && a == b->left_) {
    a = b;
    b = b->parent_;
  }
  return b;
}
void nodes::tree_node::reparent(nodes::tree_node* ptr) {
  if (parent_ == nullptr) {
    return;
  }
  if (this == parent_->left_) {
    parent_->left_ = ptr;
  } else {
    parent_->right_ = ptr;
  }
  if (ptr != nullptr) {
    ptr->parent_ = parent_;
  }
}
