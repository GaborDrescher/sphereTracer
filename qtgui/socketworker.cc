#include "socketworker.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "../lzo/minilzo.h"

static void die(const char message[])
{
	perror(message);
	exit(EXIT_FAILURE);
}

static void setNoDelay(int sock)
{
	int yes = 1;
	if(setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) != 0) {
		die("setsockopt()");
	}
}

static uint64_t get_us()
{
	struct timespec tp;
	if(clock_gettime(CLOCK_MONOTONIC_RAW, &tp) != 0) {
		die("clock_gettime()");
	}

	return ((uint64_t)tp.tv_sec) * 1000000 + ((uint64_t)tp.tv_nsec) / 1000;
}

void SocketWorker::init()
{
	width = 1024;
	height = 1024;
	nextWidth = width;
	nextHeight = height;
	nextSpheres = 8;
	nextRadius = 30;
	nextRTDepth = 16;
	nextSPP = 1;
	nextMemProtActive = 1;

	prevImg = new uint8_t[width * height * 3];
	drawImg = new uint8_t[width * height * 3];
	decompBuffer = new uint8_t[width * height * 3];

	memset(prevImg, 0, width * height * 3);

	if(sem_init(&sem, 0, 30) != 0) {
		die("sem_init");
	}
	errno = pthread_mutex_init(&dataLock, NULL);
	if(errno != 0) {
		die("pthread_mutex_init");
	}
	errno = pthread_mutex_init(&guiLock, NULL);
	if(errno != 0) {
		die("pthread_mutex_init");
	}

	const uint16_t port = 8080;
	listenSock = socket(AF_INET6, SOCK_STREAM, 0);
	if(listenSock == -1)
		die("socket()");
	//int flags = fcntl(listenSock, F_GETFD, 0);
	//if(flags == -1 || fcntl(listenSock, F_SETFD, flags | FD_CLOEXEC) != 0)
	//	perror("fcntl()");

	int yes = 1;
	if(setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0) {
		die("setsockopt()");
	}

	setNoDelay(listenSock);

	struct sockaddr_in6 address;
	memset(&address, 0, sizeof(address));
	address.sin6_family = AF_INET6;
	address.sin6_port	= htons(port);
	address.sin6_addr	= in6addr_any;
	if(bind(listenSock, (const struct sockaddr *) &address, sizeof(address)) != 0) {
		die("bind()");
	}
	if(listen(listenSock, SOMAXCONN) != 0) {
		die("listen()");
	}
}

static uint64_t decode64(uint8_t *buffer)
{
	uint64_t out = 0;
	for(uintptr_t i = 0; i < 8; ++i) {
		out |= buffer[i] << (i*8);
	}
	return out;
}

static void encode64(uint64_t val, uint8_t *buffer)
{
	for(uintptr_t i = 0; i < 8; ++i) {
		buffer[i] = (val >> (i*8)) & 0xff;
	}
}

static bool recvAll(FILE *in, uint8_t *buffer, uintptr_t len)
{
	while(len > 0) {
		uintptr_t sockRead = fread(buffer, 1, len, in);
		if(sockRead == 0) {
			return false;
		}
		buffer += sockRead;
		len -= sockRead;
	}
	return true;
}

static bool sendAll(FILE *out, uint8_t *buffer, uintptr_t len) {
	while(len > 0) {
		uintptr_t sockWrite = fwrite(buffer, 1, len, out);
		if(sockWrite == 0) {
			return false;
		}
		buffer += sockWrite;
		len -= sockWrite;
	}
	if(fflush(out) != 0) {
		die("fflush");
	}
	return true;
}

static uintptr_t my_lzo_decompress(const uint8_t *in, uintptr_t inlen, uint8_t *out, uintptr_t outlen)
{
	lzo_uint dst_len = outlen;
	int ret = lzo1x_decompress((const lzo_bytep)in, (lzo_uint)inlen, (lzo_bytep)out, &dst_len, (void*)0);
	
	if(ret != LZO_E_OK) {
		return 0;
	}
	return dst_len;
}

void SocketWorker::lock(pthread_mutex_t *mutex)
{
	errno = pthread_mutex_lock(mutex);
	if(errno != 0) {
		die("pthread_mutex_lock");
	}
}

void SocketWorker::unlock(pthread_mutex_t *mutex)
{
	errno = pthread_mutex_unlock(mutex);
	if(errno != 0) {
		die("pthread_mutex_unlock");
	}
}

void SocketWorker::signal()
{
	if(sem_post(&sem) != 0) {
		die("sem_post");
	}
}

void SocketWorker::wait()
{
	for(;;) {
		int ret = sem_wait(&sem);
		if(ret == 0) {
			break;
		}
		if(errno != EINTR) {
			die("sem_wait");
		}
		// else try again
	}
}

void SocketWorker::handleRequest(FILE *fromClient, FILE *toClient)
{
	for(;;)
	{
		uint64_t recvTime = get_us();
		uint8_t lenBuf[8];
		if(!recvAll(fromClient, lenBuf, 8)) {
			break;
		}

		uint64_t length = decode64(lenBuf);
		//printf("recv buffer of length %" PRIu64 "\n", length);
		uint8_t *buffer = nullptr;
		if(length > 0) {
			buffer = new uint8_t[length];
			if(!recvAll(fromClient, buffer, length)) {
				break;
			}
		}
		recvTime = get_us() - recvTime;
		if(recvTime == 0) {
			recvTime = 1;
		}
		printf("recv %" PRIuPTR " bytes in %" PRIu64 " us -> %" PRIu64 " kiB/s\n", length+8, recvTime, ((length+8)*1000) / recvTime);

		uintptr_t vals[7];
		lock(&guiLock);
			// read next gui vars
			vals[0] = nextWidth;
			vals[1] = nextHeight;
			vals[2] = nextRTDepth;
			vals[3] = nextSpheres;
			vals[4] = nextRadius;
			vals[5] = nextSPP;
			vals[6] = nextMemProtActive;
		unlock(&guiLock);

		uint8_t sendBuffer[8 * 7];
		for(uintptr_t i = 0; i < 7; ++i) {
			encode64(vals[i], sendBuffer + (i*8));
		}

		uint64_t sendTime = get_us();
		if(!sendAll(toClient, sendBuffer, 8 * 7)) {
			break;
		}
		sendTime = get_us() - sendTime;
		if(sendTime == 0) {
			sendTime = 1;
		}
		printf("send %" PRIuPTR " bytes in %" PRIu64 " us -> %" PRIu64 " kiB/s\n", (uintptr_t)8*7, sendTime, (8*7*1000) / sendTime);

		if(length == 0) {
			continue;
		}

		ImgData *data = new ImgData();
		uintptr_t byteIdx = 0;
		data->measurementVals = decode64(buffer + byteIdx);
		byteIdx += 8;

		for(uintptr_t i = 0; i < data->measurementVals; ++i) {
			data->measurements.append(decode64(buffer + byteIdx));
			byteIdx += 8;
		}

		uintptr_t newWidth = decode64(buffer + byteIdx);
		byteIdx += 8;

		uintptr_t newHeight = decode64(buffer + byteIdx);
		byteIdx += 8;

		uintptr_t lastPicZero = decode64(buffer + byteIdx);
		byteIdx += 8;

		uintptr_t comprPicSize = decode64(buffer + byteIdx);
		byteIdx += 8;

		if(newWidth != width || newHeight != height || lastPicZero == 1) {
			width = newWidth;
			height = newHeight;

			delete decompBuffer;
			delete drawImg;
			delete prevImg;

			prevImg = new uint8_t[width * height * 3];
			drawImg = new uint8_t[width * height * 3];
			decompBuffer = new uint8_t[width * height * 3];

			memset(prevImg, 0, width * height * 3);
		}
		const uintptr_t picSize = width * height * 3;
		data->width = width;
		data->height = height;
		uintptr_t decompSize = my_lzo_decompress(buffer + byteIdx, comprPicSize, decompBuffer, picSize);
		if(decompSize != picSize) {
			printf("decompressed size is invalid\n");
			break;
		}

		const uintptr_t rgb32Size = width * height * 4;
		uint8_t *dataImg = new uint8_t[rgb32Size];
		for(uintptr_t y = 0; y < height; ++y) {
			for(uintptr_t x = 0; x < width; ++x) {
				uintptr_t idx = (x + y * width) * 3;
				uintptr_t idx32 = (x + y * width) * 4;

				uint8_t r = decompBuffer[idx] + prevImg[idx];
				uint8_t g = decompBuffer[idx+1] + prevImg[idx+1];
				uint8_t b = decompBuffer[idx+2] + prevImg[idx+2];

				drawImg[idx] = r;
				drawImg[idx+1] = g;
				drawImg[idx+2] = b;

				dataImg[idx32] = b;
				dataImg[idx32+1] = g;
				dataImg[idx32+2] = r;
				dataImg[idx32+3] = 0xff;
			}
		}
		data->drawImg = dataImg;

		lock(&dataLock);
			imgDeque.append(data);
		unlock(&dataLock);

		delete buffer;

		uint8_t *tmp = prevImg;
		prevImg = drawImg;
		drawImg = tmp;
	
		wait();
	}
}

void SocketWorker::doWork()
{
	printf("worker working\n");
	for(;;) {
		int clientSock = accept(listenSock, NULL, NULL);
		if (clientSock == -1) {
			perror("accept()");
			continue;
		}

		printf("accepted connection\n");

		setNoDelay(clientSock);

		FILE *toClient = fdopen(clientSock, "w");
		if(toClient == NULL) {
			die("fdopen()");
		}

		int fd2 = dup(clientSock);
		if(fd2 == -1) {
			fclose(toClient);
			die("dup()");
		}

		FILE *fromClient = fdopen(fd2, "r");
		if(fromClient == NULL) {
			close(fd2);
			fclose(toClient);
			die("fdopen()");
		}

		handleRequest(fromClient, toClient);

		fflush(toClient);

		printf("closing connection\n");
		fclose(toClient);
		fclose(fromClient);
	}
}

static void* workerRun(void *socketWorkerPtr)
{
	SocketWorker *worker = (SocketWorker*)socketWorkerPtr;
	worker->doWork();
	return NULL;
}

void SocketWorker::run()
{
	pthread_t worker;
	errno = pthread_create(&worker, NULL, workerRun, this);
	if(errno != 0) {
		die("pthread_create");
	}
}
