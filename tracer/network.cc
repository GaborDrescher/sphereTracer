#include <stdlib.h>
#include <stdio.h>
#include "network.h"
#include "util.h"

#ifndef __OCTOPOS__
#	include <netdb.h>
#	include <string.h>
#	include <unistd.h>
#	include <sys/socket.h>
#	include <errno.h>
#endif

#ifdef __OCTOPOS__
void Network::jobFinished()
{
	LockItem item;
	spinlock.lock(&item);
	asyncJobsReady += 1;
	if(asyncJobsReady == asyncJobsTriggered) {
		sem.signal();
	}
	spinlock.unlock(&item);
}
#endif

void Network::syncSend(void *buffer, uintptr_t size)
{
	(void)buffer;
	(void)size;

	#ifdef __OCTOPOS__
	if(size == 0) {
		return;
	}
	uint8_t *start = (uint8_t*)buffer;
	error_t err;
	os::ipc::Future<uintptr_t> fsend;
	err = os::net::lwIP::TCP::send(tcpid, fsend, start, size);
	//fprintf(stderr, "os::net::lwIP::TCP::send (%" PRIuPTR ", %p, %p, %lu) = %u\n", tcpid, &fsend, start, size, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 5\n");
	}

	err = fsend.wait(size);
	//fprintf(stderr, "future@%p.wait (sent:%lu) = %u\n", &fsend, size, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 6\n");
	}
	#endif
}

void Network::syncRecv(void *buffer, uintptr_t size)
{
	(void)buffer;
	(void)size;

	#ifdef __OCTOPOS__
	if(size == 0) {
		return;
	}
	uint8_t *start = (uint8_t*)buffer;
	error_t err;
	os::ipc::Future<uintptr_t> frecv;
	err = os::net::lwIP::TCP::recv(tcpid, frecv, start, size);
	//fprintf(stderr, "os::net::lwIP::TCP::frecv (%" PRIuPTR ", %p, %p, %lu) = %u\n", tcpid, &frecv, start, size, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 7\n");
	}

	err = frecv.wait(size);
	//fprintf(stderr, "future@%p.wait (recv:%lu) = %u\n", &frecv, size, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 8\n");
	}
	#endif
}

void Network::doNetworkInit()
{
	#ifdef __OCTOPOS__
	uint32_t ip = LWIP_MAKEU32(132, 0, 168, 192);
	ip_addr_t addr;
	ip_addr_set_ip4_u32 (&addr, ip);
	const uint8_t *const _a = reinterpret_cast<const uint8_t *>(&ip);
	uint16_t port = 8080;

	os::ipc::Future <os::net::lwIP::TCP::ID> fid;
	error_t err;

	err = os::net::lwIP::TCP::create (fid);
	fprintf(stderr, "os::net::lwIP::TCP::create (%p) = %u\n", &fid, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 1\n");
	}
	err = fid.wait(tcpid);
	fprintf(stderr, "future@%p.wait(listen:%" PRIuPTR ") = %u\n", &fid, tcpid, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 2\n");
	}

	os::ipc::Future<void> fconnect;
	err = os::net::lwIP::TCP::connect(tcpid, fconnect, addr, port);
	fprintf(stderr, "os::net::lwIP::TCP::connect (%" PRIuPTR ", %p, %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16 ") = %u\n", tcpid, &fconnect, _a[0], _a[1], _a[2], _a[3], port, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 3\n");
	}

	err = fconnect.wait();
	fprintf(stderr, "future@%p.wait () = %u\n", &fconnect, err);
	if (err != ERROR_NO_ERROR) {
		Util::die("network error 4\n");
	}
	#endif
}

void Network::doNetWork()
{
	#ifdef __OCTOPOS__
	for(;;) {
		LockItem item;
		spinlock.lock(&item);
		Job *job = queue.dequeue();
		spinlock.unlock(&item);

		if(job != nullptr) {
			if(job->send) {
				//fprintf(stderr, "net worker sending 0x%p\n", job);
				syncSend(job->buffer, job->size);
			}
			else {
				//fprintf(stderr, "net worker recving 0x%p\n", job);
				syncRecv(job->buffer, job->size);
			}
			free(job);

			jobFinished();
		}
	}
	#endif
}

#ifdef __OCTOPOS__
static void netWorker(void *rawNet)
{
	Network *net = (Network*)rawNet;
	net->doNetWork();
}
#endif

void Network::init()
{
	#ifdef __OCTOPOS__
		spinlock.init();
		queue.init();
		sem.init();
		this->asyncJobsTriggered = 0;
		this->asyncJobsReady = 0;


		doNetworkInit();

		//myClaim = claim_construct();

		//if(invade_simple(myClaim, 1) != 0) {
		//	Util::die("invade_simple in Network::init failed\n");
		//}

		//simple_ilet ilet;
		//simple_ilet_init(&ilet, netWorker, this);

		//if(infect(myClaim, &ilet, 1) != 0) {
		//	Util::die("infect in Network::init failed\n");
		//}
	#else
		const char *host = "192.168.46.136";
		const char *port = "8080";

		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));

		hints.ai_flags    = AI_ADDRCONFIG;
		hints.ai_family   = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		struct addrinfo *result;
		int error = getaddrinfo(host, port, &hints, &result);
		if (error != 0) {
			if (error == EAI_SYSTEM) {
				Util::perror_die("getaddrinfo");
			}
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
			Util::die();
		}
		struct addrinfo *info;
		int sock = -1;
		for (info = result; info != NULL; info = info->ai_next) {
			sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if (sock == -1)
				continue;
			if (connect(sock, info->ai_addr, info->ai_addrlen) == 0)
				break;
			close(sock);
		}
		if(info == NULL) {
			Util::perror_die("connect");
		}
		freeaddrinfo(result);

		conIn = fdopen(sock, "r");
		if (conIn == NULL) Util::perror_die("fdopen");
		sock = dup(sock);
		if (sock == -1) Util::perror_die("dup");
		conOut = fdopen(sock, "w");
		if (conOut == NULL) Util::perror_die("fdopen");
	#endif
}

void Network::send(void *sendBuffer, uintptr_t sendSize)
{
	#ifdef __OCTOPOS__
	//Job *job = (Job*)malloc_or_die(sizeof(Job));
	//job->buffer = sendBuffer;
	//job->size = sendSize;
	//job->send = true;

	//LockItem item;
	//spinlock.lock(&item);
	//	queue.enqueue(job);
	//	asyncJobsTriggered += 1;
	//spinlock.unlock(&item);
	syncSend(sendBuffer, sendSize);

	#else
	while(sendSize > 0) {
		size_t written = fwrite(sendBuffer, 1, sendSize, conOut);
		if(written == 0) {
			Util::die("fwrite");
		}
		sendBuffer = (void*)(((uint8_t*)sendBuffer) + written);
		sendSize -= written;
	}
	if(fflush(conOut) != 0) {
		Util::die("fflush");
	}
	#endif
}

void Network::recv(void *recvBuffer, uintptr_t recvSize)
{
	#ifdef __OCTOPOS__
	//Job *job = (Job*)malloc_or_die(sizeof(Job));
	//job->buffer = recvBuffer;
	//job->size = recvSize;
	//job->send = false;

	//LockItem item;
	//spinlock.lock(&item);
	//	queue.enqueue(job);
	//	asyncJobsTriggered += 1;
	//spinlock.unlock(&item);
	syncRecv(recvBuffer, recvSize);

	#else
	while(recvSize > 0) {
		size_t recvd = fread(recvBuffer, 1, recvSize, conIn);
		if(recvd == 0) {
			Util::die("fread");
		}
		recvBuffer = (void*)(((uint8_t*)recvBuffer) + recvd);
		recvSize -= recvd;
	}
	#endif
}

void Network::wait()
{
	//#ifdef __OCTOPOS__
	//sem.wait();

	//LockItem item;
	//spinlock.lock(&item);
	//	sem.init();
	//	this->asyncJobsTriggered = 0;
	//	this->asyncJobsReady = 0;
	//spinlock.unlock(&item);
	//#endif
}
