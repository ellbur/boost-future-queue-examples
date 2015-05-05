
#include "BlockingQueue.hpp"
#include "OneByOneBlockingQueue.hpp"

using boost::when_any;
using boost::future;
using boost::thread;
using std::tuple;

template<class T> struct MultixedBlockingReader : public BlockingReader<optional<T>> {
  MultixedBlockingReader() :
    ourClosed(),
    current(&ourClosed),
    feederQueue(2, 10)
  { }
  
  void operator=(BlockingReader<optional<T>> &nextFeeder) {
    feederQueue.feed(&nextFeeder);
  }

  void closeSink() {
    feederQueue.closeSink();
  }
  
  future<optional<T>> next() {
    return current->next().then([this](future<optional<T>> futureHead) -> future<optional<T>> {
      optional<T> head = std::move(futureHead.get());
      if (head) {
        return make_ready_future(head);
      }
      else {
        return feederQueue.next().then([this](future<optional<BlockingReader<optional<T>>*>> nextFeeder) -> future<optional<T>> {
          optional<BlockingReader<optional<T>>*> it = std::move(nextFeeder.get());
          if (it) {
            current = it.get();
            return this->next();
          }
          else {
            return make_ready_future(optional<T>());
          }
        }).unwrap();
      }
    }).unwrap();
  }
  
  private:
    struct ClosedBlockingReader : public BlockingReader<optional<T>> {
      future<optional<T>> next() {
        return make_ready_future(optional<T>());
      }
    };
  
    ClosedBlockingReader ourClosed;
    BlockingReader<optional<T>> *current;
    OneByOneBlockingQueue<BlockingReader<optional<T>>*> feederQueue;
};

