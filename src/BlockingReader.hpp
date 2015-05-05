
#pragma once

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

using boost::future;

template<class T> struct BlockingReader {
  virtual future<T> next() = 0;
};

