#include "comm_gld_ns3.h"

//virtual CommGldNs3<class T>::~CommGldNs3() {
CommGldNs3<class T>::~CommGldNs3() {
		delete receive_queue_;
		pthread_mutex_destroy(&lock_);
		pthread_cond_destroy(&empty_);
		pthread_cancel(receive_thread_);
	}
	
int CommGldNs3<class T>::getTx() {
  return Tx;
}

int CommGldNs3<class T>::getRx() {
  return Rx;
}

void  CommGldNs3<class T>::getTime(struct timespec* pTime, uint64 iTimeOut) {
#ifdef WIN32
		struct _timeb tb;
		_ftime_s(&tb);
		pTime->tv_nsec = tb.millitm * 1000000;
		pTime->tv_sec = tb.time;
#else
#ifdef __MACH__ // OS X does not have clock_gettime, using clock_get_time
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		pTime->tv_sec = mts.tv_sec;
		pTime->tv_nsec = mts.tv_nsec;
#else
		clock_gettime(CLOCK_REALTIME, pTime);
#endif
#endif

		uint64 curr_time = pTime->tv_sec * (ONE_K * ONE_K * ONE_K) + pTime->tv_nsec;
		uint64 add_time = iTimeOut * ONE_K * ONE_K;

		uint64 new_time = curr_time + add_time;

		pTime->tv_sec    = new_time / (ONE_K * ONE_K * ONE_K);
		pTime->tv_nsec   = new_time % (ONE_K * ONE_K * ONE_K);
	}
