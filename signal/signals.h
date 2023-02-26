#pragma once
#include "intrusive_list.h"
#include <functional>

// Чтобы не было коллизий с UNIX-сигналами реализация вынесена в неймспейс, по
// той же причине изменено и название файла
namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  struct connection;

private:
  struct connection_tag {};
  using slot_t = std::function<void(Args...)>;
  using connections_list = intrusive::list<connection, connection_tag>;
  using list_const_it = typename connections_list::const_iterator;

public:
  struct connection : intrusive::list_element<connection_tag> {

    connection() = default;

  private:
    connection(signal* sig_, slot_t slot_) : sig(sig_), slot(std::move(slot_)) {
      sig->connections.insert(sig->connections.end(), *this);
    }

  public:
    connection(connection&& other) : sig(other.sig), slot(std::move(other.slot)) {
      if (sig != nullptr) {
        sig->connections.insert(&other, *this);
        other.disconnect();
      }
    }

    connection& operator=(connection&& other) {
      if (this == &other) {
        return *this;
      }
      disconnect();
      slot = std::move(other.slot);
      sig = other.sig;
      if (sig != nullptr) {
        sig->connections.insert(&other, *this);
        other.disconnect();
      }
      return *this;
    }

    void disconnect() {
      if (sig != nullptr) {
        for (auto t = sig->tail; t != nullptr; t = t->prev) {
          if (t->cur == this) {
            t->cur++;
          }
        }
        sig->connections.erase(this);
        sig = nullptr;
      }
    }

    void operator()(Args... args) const {
      slot(args...);
    }

    ~connection() {
      disconnect();
    }

    friend signal<void(Args...)>;

  private:
    signal* sig{nullptr};
    slot_t slot;
  };

  signal() = default;

  signal(signal const&) = delete;
  signal& operator=(signal const&) = delete;

  ~signal() {
    for (; tail != nullptr; tail = tail->prev) {
      tail->sig = nullptr;
    }
    for (auto& con : connections) {
      con.sig = nullptr;
      con.slot = [](Args...) {};
    }
  }

  connection connect(std::function<void(Args...)> slot) noexcept {
    return connection(this, std::move(slot));
  }

  void operator()(Args... args) const {
    iterator_holder holder(this);
    while (holder.cur != connections.end()) {
      list_const_it copy = holder.cur;
      holder.cur++;
      (*copy)(args...);
      if (holder.sig == nullptr) {
        return;
      }
    }
  }

private:
  struct iterator_holder {
    explicit iterator_holder(signal const* s) : sig(s), cur(s->connections.begin()), prev(s->tail) {
      sig->tail = this;
    }
    ~iterator_holder() {
      if (sig != nullptr) {
        sig->tail = prev;
      }
    }
    signal const* sig;
    list_const_it cur;
    iterator_holder* prev;
  };
  connections_list connections;
  mutable iterator_holder* tail{nullptr};
};

} // namespace signals
