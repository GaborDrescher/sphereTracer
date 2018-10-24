#ifndef DEMO_NETWORK_HEADER
#define DEMO_NETWORK_HEADER

#include <inttypes.h>
#ifdef __OCTOPOS__
#	include "octopos.h"
#	include "lib/adt/MCSLock.h"
#	include "lib/adt/Queue.h"
#	include "hw/hal/Atomic.h"
#	include "os/net/lwIP/TCP.h"
#	include "os/ipc/Future.h"

	using namespace lib::adt;
	using namespace hw::hal;
#else
#	include "stdio.h"
#endif

class Network
{
	private:
	#ifdef __OCTOPOS__
	typedef MCSLock::Item LockItem;

	class Job
	{
		public:
		QueueNode<Job> qNode;

		void *buffer;
		uintptr_t size;
		bool send;
	};

	class SSSem
	{
		private:
		binary_signal bSignal;

		public:
		void init()
		{
			binary_signal_init(&bSignal);
		}

		void signal()
		{
			binary_signal_signal(&bSignal);
		}

		void wait()
		{
			binary_signal_wait(&bSignal);
		}
	};


	claim_t myClaim;
	MCSLock spinlock;
	Queue<Job, &Job::qNode> queue;
	SSSem sem;
	uintptr_t asyncJobsTriggered;
	uintptr_t asyncJobsReady;
	os::net::lwIP::TCP::ID tcpid;

	#else
	FILE *conIn;
	FILE *conOut;
	#endif

	void syncSend(void *buffer, uintptr_t size);
	void syncRecv(void *buffer, uintptr_t size);
	void doNetworkInit();
	void jobFinished();

	public:
	void init();
	void send(void *sendBuffer, uintptr_t sendSize);
	void recv(void *recvBuffer, uintptr_t recvSize);
	void wait();
	void doNetWork();
};

#endif /* DEMO_NETWORK_HEADER */
