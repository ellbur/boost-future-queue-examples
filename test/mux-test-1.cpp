
#include "../src/BlockingQueue.hpp"
#include "../src/MultiplexedBlockingReader.hpp"
#include <iostream>

using boost::thread;
using std::cout;

int main() {
  BlockingQueue<int> q1(2, 10);
  BlockingQueue<int> q2(2, 100);
  
  MultixedBlockingReader<vector<int>> mux;
  
  thread consumer([&mux] {
    for (;;) {
      optional<vector<int>> next = std::move(mux.next().get());
      if (next) {
        for (int x : next.get()) {
          cout << x << "\n";
        }
      }
      else {
        break;
      }
    }
  });

  q1.feed({1});
  mux = q1;
  q1.feed({2});
  q1.feed({3});
  mux = q2;
  q1.feed({4});
  q1.closeSink();
  q2.feed({5});
  q2.feed({6});
  for (int i=7; i<100; i++) {
    q2.feed({i});
  }
  q2.closeSink();
  mux.closeSink();
  
  consumer.join();
  return 0;
}

