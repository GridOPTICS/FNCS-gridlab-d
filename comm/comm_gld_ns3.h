/*
 * comm_gld_ns3.hh
 *
 *  Created on: Apr 17, 2012
 *      Author: basu161
 */

#ifndef _COMM_GLD_NS3_HH_
#define _COMM_GLD_NS3_HH_

#include "comm.h"

#ifdef WIN32
#include <time.h>
#include <sys/timeb.h>
#else
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#else
#include <time.h>
#include <sys/time.h>
#endif
#endif

#include <math.h>
#include <stdlib.h>
#include <queue>
#include "pthread.h"
#include <iostream>

template <class T> class CommGldNs3 {
private:
	static const int ONE_K = 1000;
	static CommGldNs3<T>* comm_obj_;
	std::queue<T*>* receive_queue_;
	pthread_mutex_t lock_;
	pthread_cond_t empty_;
	pthread_t receive_thread_;
	int Tx, Rx;
public:
  ~CommGldNs3() {
		delete receive_queue_;
		pthread_mutex_destroy(&lock_);
		pthread_cond_destroy(&empty_);
		pthread_cancel(receive_thread_);
	}

	static CommGldNs3<T>* CommGldNs3Factory() {
		if(NULL == comm_obj_) {
			comm_obj_ = new CommGldNs3<T>();
		}
		return comm_obj_;
	}

	int sendMsg(T *msg_obj) {
		Tx++;
		uint32 msg_len;
		char *msg;
		if(msg_obj == 0){
			return -1;
		}
		msg = msg_obj->serialize(&msg_len);
		if(msg == 0){
			return -1; // malfomed message
		}
//#ifdef REALLYUSEMPI
		//std::cout << "Sending to MPI from GLD" << std::endl;
		// exit(0);
		gl_output("sendMsg(): '%s'", msg);
		MPI_Send((void*)&msg_len, 1, MPI_UNSIGNED, 1, 2, MPI_COMM_WORLD);
		int toReturn=MPI_Send((void*)msg, msg_len, MPI_BYTE, 1, 2, MPI_COMM_WORLD); // MPI_SUCCESS == 0
		free(msg);
		return toReturn;
		// TODO handle what exactly MPI is telling us
//#else
//		return 0;
//#endif
	}

	const T* receiveMsg(void) {
		Rx++;
		return getMsg();
	}

	static void *thread_routine(void* args) {
		CommGldNs3<T>* comm_obj = (CommGldNs3<T>*)args;
		MPI_Status status;
		int rv = 0;
		const int source = 1;
		const int tag = 2;

		while(1) {
			uint32 msg_len;
//#ifdef REALLYUSEMPI  
			//std::cout << "In thread" << std::endl;
			rv = MPI_Recv((void*)&msg_len, 1, MPI_UNSIGNED, source, tag, MPI_COMM_WORLD, &status);
//#endif
			switch(rv){
				case MPI_SUCCESS:
					// working properly
					break;
				case MPI_ERR_COMM:
					gl_error("thread_routine(): invalid MPI communicator 'MPI_COMM_WORLD' when reading msg length"); // uh, what?
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_TYPE:
					gl_error("thread_routine(): invalid MPI type 'MPI_UNSIGNED' when reading msg length"); // uh, what?
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_COUNT:
					gl_error("thread_routine(): invalid MPI count '1' when reading msg length"); // uh, what?
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_TAG:
					gl_error("thread_routine(): invalid MPI tag '%i' when reading msg length", tag);
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_RANK:
					gl_error("thread_routine(): invalid MPI source rank '%i' when reading msg length", source);
					exit(-42); // unrecoverable
					break;
				default:
					gl_error("thread_routine(): an unexpected result returned from MPI_Recv(&msg_len) when reading msg length");
					continue;
			}
			//std::cout << "ns3 message len " << msg_len << std::endl;
			char* buff = new char[msg_len+1];
			
//#ifdef REALLYUSEMPI
			rv = MPI_Recv(buff, msg_len, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
//#endif
			switch(rv){
				case MPI_SUCCESS:
					// working properly
					break;
				case MPI_ERR_COMM:
					gl_error("thread_routine(): invalid MPI communicator 'MPI_COMM_WORLD' when reading msg data"); // uh, what?
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_TYPE:
					gl_error("thread_routine(): invalid MPI type 'MPI_BYTE' when reading msg data"); // uh, what?
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_COUNT:
					gl_error("thread_routine(): invalid MPI count '%i' when reading msg data", msg_len);
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_TAG:
					gl_error("thread_routine(): invalid MPI tag '%i' when reading msg data", tag);
					exit(-42); // unrecoverable
					break;
				case MPI_ERR_RANK:
					gl_error("thread_routine(): invalid MPI source rank '%i' when reading msg data", source);
					exit(-42); // unrecoverable
					break;
				default:
					gl_error("thread_routine(): an unexpected result returned from MPI_Recv(&msg_len) when reading msg data");
					continue;
			}
			buff[msg_len] = '\0';
			int iMsgLen = 0;
			rv = MPI_Get_count(&status, MPI_BYTE, &iMsgLen);
			switch(rv){
				case MPI_SUCCESS:
					break;
				case MPI_ERR_TYPE:
					gl_error("thread_routine(): MPI_BYTE not valid for MPI_Get_count()"); // uh, what?
					exit(-42);
					break;
			}
//			T* msg_obj = new T(buff,msg_len+1);
			if(iMsgLen < 1){
				gl_warning("thread_routine(): MPI_Get_count() result less than one byte!");
				continue;
			}
			T* msg_obj = new T();
			if(0 == msg_obj->deserialize(buff, iMsgLen+1)){
				gl_error("thread_routine(): failure to deserialize the message '%s' (%i), ignoring", buff, iMsgLen);
				delete msg_obj;
				delete buff;
				continue;
				//exit(-42);
			}
			comm_obj->addMsg(msg_obj);
			delete buff;
		}

		return NULL;
	}

	CommGldNs3() {
		receive_queue_ = new std::queue<T*>();
		pthread_mutex_init(&lock_, NULL);
		pthread_cond_init(&empty_, NULL);

		if (pthread_create(&receive_thread_, NULL, thread_routine, this)){
			//std::cout << "ERROR: return code from pthread_create() is " <<  rc << std::endl;
			exit(-1);
		}
		Tx=0;
		Rx=0;
	}

	/*
	 * Converts the waittime into the 'timespec' structure required for pthread_cond_timedwait
	 */
	void  getTime(struct timespec* pTime, uint64 iTimeOut) {
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

	
	void addMsg(T* msg) {

		pthread_mutex_lock(&lock_);

		receive_queue_->push(msg);
		//std::cout << "SIZE!!" << receive_queue_->size() << std::endl;
		if(1 == receive_queue_->size()) {
			pthread_cond_broadcast(&empty_);
		}

		pthread_mutex_unlock(&lock_);
	}

	T* getMsg(void) {

		pthread_mutex_lock(&lock_);
		while(!receive_queue_->size()) {
			pthread_cond_wait(&empty_, &lock_);
		}
		T* msg = receive_queue_->front();
		receive_queue_->pop();
		pthread_mutex_unlock(&lock_);

		return msg;
	}

	T* getMsgWithTimeOut(uint64 iTimeOut) {
		T* msg = NULL;

		pthread_mutex_lock(&lock_);

		{
			struct timespec wait_time;
			getTime(&wait_time, iTimeOut);

			int rc = 0;
			while(!receive_queue_->size() && (0 == rc)) {
				rc = pthread_cond_timedwait(&empty_, &lock_, &wait_time);
			}
		}

		if(receive_queue_->size()) {
			msg = receive_queue_->front();
			receive_queue_->pop();
		}
		pthread_mutex_unlock(&lock_);

		return msg;
	}
	
	
	T* peekMsgWithTimeOut(uint64 iTimeOut) {
		T* msg = NULL;

		pthread_mutex_lock(&lock_);

		{
			struct timespec wait_time;
			getTime(&wait_time, iTimeOut);
			int rc = 0;
			while(!receive_queue_->size() && (0 == rc)) {
				rc = pthread_cond_timedwait(&empty_, &lock_, &wait_time);
			}
		}

		if(receive_queue_->size()) {
			msg = receive_queue_->front();
		
		}
		pthread_mutex_unlock(&lock_);

		return msg;
	}
	
	T* peekMsg(void) {

		pthread_mutex_lock(&lock_);
		while(!receive_queue_->size()) {
			pthread_cond_wait(&empty_, &lock_);
		}
		T* msg = receive_queue_->front();
		pthread_mutex_unlock(&lock_);

		return msg;
	}
	
	int getTx(){
	  return Tx;
	}
	int getRx(){
	  return Rx;
	}
};

template<class T> CommGldNs3<T>* CommGldNs3<T>::comm_obj_ = NULL;

#endif /* _COMM_GLD_NS3_HH_ */
