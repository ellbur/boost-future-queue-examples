
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <iostream>
#include <tuple>

using boost::future;
using boost::shared_future;
using boost::make_ready_future;
using boost::future_status;
using boost::wait_for_any;
using boost::when_any;
using boost::promise;
using boost::mutex;
using boost::unique_lock;
using boost::shared_lock;
using std::vector;
using boost::circular_buffer_space_optimized;
using boost::condition_variable;
using boost::thread;
using std::cout;
using std::cin;
using std::tuple;

struct StreamClosed { };

template<class T> struct EventBlock {
  virtual shared_future<T> next() = 0;
};

struct Unit {
};

template<class T> struct BlockingQueue : public EventBlock<vector<T>> {
  BlockingQueue(size_t minSize, size_t maxSize) :
    lock(),
    wakeup(),
    waiting(false),
    buffer(typename circular_buffer_space_optimized<T>::capacity_type { maxSize, minSize }),
    waitingFuture(),
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
      wakeup.set_value({ });
      wakeup = promise<Unit>();
      waiting = false;
    }
  }
  
  void feed(vector<T> const& elements) {
    unique_lock<mutex> _(lock);
    copy(elements.begin(), elements.end(), back_inserter(buffer));
    
    if (waiting) {
      wakeup.set_value({ });
      wakeup = promise<Unit>();
      waiting = false;
    }
  }
  
  shared_future<vector<T>> next() {
    unique_lock<mutex> _(lock);
    
    
    if (buffer.size() != 0) {
      return make_ready_future(drain());
    }
    else {
      if (closed) {
        throw StreamClosed();
      }
      else {
        if (!waiting) {
          shared_future<Unit> onWakeup = wakeup.get_future();
          waitingFuture = onWakeup.then([this](shared_future<Unit> _f) {
            unique_lock<mutex> _(lock);
            return drain();
          });
        }
        
        // Arm the event!
        waiting = true;

        return waitingFuture;
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
    promise<Unit> wakeup;
    bool waiting;
    circular_buffer_space_optimized<T> buffer;
    shared_future<vector<int>> waitingFuture;
    bool closed;
};

int main() {
  promise<Unit> consumerTermination;
  future<Unit> consumerTerminated = consumerTermination.get_future();
  
  future<Unit> consoleTerminated = boost::async([]() -> Unit { cin.getline(NULL, 0); return { }; });
  
  shared_future<Unit> terminated = when_any(std::move(consumerTerminated), std::move(consoleTerminated))
    .then([](future<tuple<future<Unit>,future<Unit>>> t) { return Unit { }; });
  
  BlockingQueue<int> queue(2, 10);
  
  thread producer([&queue,&terminated] {
    for (int i=0; ; i++) {
      if (terminated.wait_for(boost::chrono::milliseconds(500)) == future_status::ready) {
        // Just feels more right somehow.
        terminated.get();
        queue.closeSink();
        break;
      }
      
      queue.feed({ i });
    }
  });

  thread consumer([&queue, &terminated] {
    int total = 0;
    for (;;) {
      shared_future<vector<int>> queueNext = queue.next();
      unsigned which = wait_for_any(queueNext, terminated);
      if (which == 0) {
        for (int x : queueNext.get()) {
          cout << x << "\n";
          total += 1;

          if (total >= 20)
            goto end;
        }
      }
      else if (which == 1) {
        goto end;
      }
    }
    
    end: ;
  });
  
  thread consumer2([&queue] {
    for (;;) {
      try {
        for (int x : queue.next().get()) {
          cout << "[" << x << "]\n";
        }
      }
      catch (StreamClosed _) {
        break;
      }
    }
  });

  consumer.join();
  consumerTermination.set_value({});
  producer.join();

  return 0;
}

