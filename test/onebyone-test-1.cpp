
#include "../src/OneByOneBlockingQueue.hpp"
#include <iostream>

using boost::thread;
using std::cout;

int main() {
  OneByOneBlockingQueue<int> q(2, 10);

  thread consumer([&]() {
    for (;;) {
      optional<int> next = q.next().get();
      if (next) {
        cout << next.get() << "\n";
      }
      else {
        break;
      }
    }
  });

  q.feed(1);
  q.feed(2);
  q.feed(3);
  q.closeSink();

  consumer.join();

  return 0;
}

