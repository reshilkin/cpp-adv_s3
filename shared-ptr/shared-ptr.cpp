#include "shared-ptr.h"

using namespace details;

void control_block::strong_inc() {
  strong_ref_++;
  weak_inc();
}
void control_block::weak_inc() {
  weak_ref_++;
}
void control_block::strong_dec() {
  strong_ref_--;
  if (strong_ref_ == 0) {
    unlink();
  }
  weak_dec();
}
void control_block::weak_dec() {
  weak_ref_--;
  if (weak_ref_ == 0) {
    delete this;
  }
}
