
#pragma once

#include "BlockingReader.hpp"
#include <vector>
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>

using std::vector;
using boost::unique_lock;
using boost::mutex;
using boost::promise;
using boost::circular_buffer_space_optimized;
using boost::make_ready_future;
using boost::optional;

template<class T> struct BlockingQueue : public BlockingReader<optional<vector<T>>> {
  BlockingQueue(size_t minSize, size_t maxSize) :
    lock(),
    wakeup(),
    waiting(false),
    buffer(typename circular_buffer_space_optimized<T>::capacity_type { maxSize, minSize }),
    closed(false)
  {
  }

  ~BlockingQueue() {
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
  
  void feed(vector<T> const& elements) {
    unique_lock<mutex> _(lock);
    copy(elements.begin(), elements.end(), back_inserter(buffer));
    
    if (waiting) {
      wakeup.set_value();
      waiting = false;
    }
  }
  
  future<optional<vector<T>>> next() {
    unique_lock<mutex> _(lock);
    
    if (buffer.size() != 0) {
      return make_ready_future(optional<vector<int>>(drain()));
    }
    else {
      if (closed) {
        return make_ready_future(optional<vector<T>>());
      }
      else {
        waiting = true;
        wakeup = promise<void>();
        return wakeup.get_future().then([this](future<void> _f) {
          unique_lock<mutex> _(lock);
          return optional<vector<int>>(drain());
        });
      }
    }
  }

  private:

    vector<T> drain() {
      vector<T> sink;
      size_t amountToRead = buffer.size();
      for (size_t i=0; i<amountToRead; i++) {
        sink.push_back(buffer.front());
        buffer.pop_front();
      }
      return sink;
    }
  
  private:
    mutex lock;
    promise<void> wakeup;
    bool waiting;
    circular_buffer_space_optimized<T> buffer;
    bool closed;
};

