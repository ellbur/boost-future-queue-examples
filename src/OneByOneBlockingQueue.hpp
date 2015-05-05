
#pragma once

#include "BlockingReader.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>

using boost::unique_lock;
using boost::mutex;
using boost::promise;
using boost::circular_buffer_space_optimized;
using boost::make_ready_future;
using boost::optional;

template<class T> struct OneByOneBlockingQueue : public BlockingReader<optional<T>> {
  OneByOneBlockingQueue(size_t minSize, size_t maxSize) :
    lock(),
    wakeup(),
    waiting(false),
    buffer(typename circular_buffer_space_optimized<T>::capacity_type { maxSize, minSize }),
    closed(false)
  {
  }

  ~OneByOneBlockingQueue() {
    closeSink();
  }

  void closeSink() {
    unique_lock<mutex> _(lock);
    closed = true;
    if (waiting) {
      wakeup.set_value();
      waiting = false;
    }
  }
  
  void feed(T const& element) {
    unique_lock<mutex> _(lock);
    buffer.push_back(element);
    
    if (waiting) {
      wakeup.set_value();
      waiting = false;
    }
  }
  
  future<optional<T>> next() {
    unique_lock<mutex> _(lock);
    
    if (buffer.size() != 0) {
      auto it = std::move(buffer.front());
      buffer.pop_front();
      return make_ready_future(optional<T>(std::move(it)));
    }
    else {
      if (closed) {
        return make_ready_future(optional<T>());
      }
      else {
        waiting = true;
        wakeup = promise<void>();
        return wakeup.get_future().then([this](future<void> _f) {
          unique_lock<mutex> _(lock);
          auto it = std::move(buffer.front());
          buffer.pop_front();
          return optional<T>(it);
        });
      }
    }
  }

  private:
    mutex lock;
    promise<void> wakeup;
    bool waiting;
    circular_buffer_space_optimized<T> buffer;
    bool closed;
};

