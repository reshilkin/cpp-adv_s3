#include "intrusive_list.h"

intrusive::list_element_base::list_element_base() : prev(this), next(this) {}

intrusive::list_element_base::list_element_base(
    const intrusive::list_element_base&)
    : list_element_base() {}

intrusive::list_element_base::list_element_base(
    intrusive::list_element_base&& other)
    : list_element_base() {
  *this = std::move(other);
}

intrusive::list_element_base&
intrusive::list_element_base::operator=(intrusive::list_element_base&& other) {
  unlink();
  if (other.next != &other) {
    other.prev->link_forward(this);
    link_forward(other.next);
    other.link_to_yourself();
  }
  return *this;
}

void intrusive::list_element_base::unlink() {
  next->prev = prev;
  prev->next = next;
  link_to_yourself();
}

void intrusive::list_element_base::link_to_yourself() {
  prev = this;
  next = this;
}

intrusive::list_element_base::~list_element_base() {
  unlink();
}
void intrusive::list_element_base::link_forward(
    intrusive::list_element_base* other) {
  next = other;
  other->prev = this;
}
