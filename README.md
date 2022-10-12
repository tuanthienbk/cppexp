### cppexp
Experiments on the latest C++ features. The code is copied and modified from various sources !!

* threadpool.h : simple thread pool with fix number of threads, simple producer-consumer with a thread-safe queue using spinlock
* lock.h: spinlock and read-write lock
* generator.h: generator model (push-based) using coroutine `co_yield` and a bunch of custom range-view models so that it works similar to (pull-based) ranges
* stream.h: abtract class to `start`, `stop` the stream and give a (async) generator to get the data from the stream

#### TODOS:
* source.h: observable-observer that calls the subscribed callbacks whenver its content is modified
