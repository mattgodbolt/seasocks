#ifndef MUTEX_H_
#define MUTEX_H_

#include <stdexcept>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace SeaSocks {

// "Borrowed" from boost to save a dependency on the whole thread library.
class Mutex: public boost::noncopyable {
	pthread_mutex_t m;
public:
	Mutex() {
		int res = pthread_mutex_init(&m, NULL);
		if (res) {
			throw std::runtime_error("Couldn't initialize mutex");
		}
	}
	~Mutex() {
		BOOST_VERIFY(!pthread_mutex_destroy(&m));
	}

	void lock() {
		BOOST_VERIFY(!pthread_mutex_lock(&m));
	}

	void unlock() {
		BOOST_VERIFY(!pthread_mutex_unlock(&m));
	}
};

class LockGuard : public boost::noncopyable{
	Mutex& m;
public:
	explicit LockGuard(Mutex& m_) : m(m_) {
		m.lock();
	}
	~LockGuard() {
		m.unlock();
	}
};

}

#endif /* MUTEX_H_ */
