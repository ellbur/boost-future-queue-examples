
#include "../src/BlockingQueue.hpp"

#include <boost/thread.hpp>
#include <iostream>
#include <tuple>

using boost::thread;
using boost::future;
using boost::future_status;
using std::cout;
using std::cin;
using std::tuple;
using boost::optional;
using boost::shared_future;

int main() {
  promise<void> consumerTermination;
  future<void> consumerTerminated = consumerTermination.get_future();
  
  future<void> consoleTerminated = boost::async([]() { cin.getline(NULL, 0); });
  
  shared_future<void> terminated = when_any(std::move(consumerTerminated), std::move(consoleTerminated))
    .then([](future<tuple<future<void>,future<void>>> t) { });
  
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
      future<optional<vector<int>>> queueNext = queue.next();
      unsigned which = wait_for_any(queueNext, terminated);
      if (which == 0) {
        optional<vector<int>> batch = queueNext.get();
        if (batch) {
          for (int x : batch.get()) {
            cout << x << "\n";
            total += 1;

            if (total >= 5)
              goto end;
          }
        }
        else {
          goto end;
        }
      }
      else if (which == 1) {
        goto end;
      }
    }
    
    end: ;
  });
  
  consumer.join();
  consumerTermination.set_value();
  producer.join();

  return 0;
}

