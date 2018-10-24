#ifndef DEMO_SOCKET_WORKER
#define DEMO_SOCKET_WORKER

#include <pthread.h>
#include <semaphore.h>
#include <inttypes.h>
#include <QVector>
#include <QList>

class ImgData
{
	public:
	uintptr_t measurementVals;
	QVector<uintptr_t> measurements;

	uintptr_t width;
	uintptr_t height;
	uint8_t *drawImg;

	ImgData()
	{
		measurementVals = 0;
		width = 0;
		height = 0;
		drawImg = nullptr;
	}
};

class SocketWorker
{
	private:
	int listenSock;
	void handleRequest(FILE *fromClient, FILE *toClient);

	public:
	uintptr_t width;
	uintptr_t height;
	uintptr_t nextWidth;
	uintptr_t nextHeight;
	uintptr_t nextSpheres;
	uintptr_t nextRadius;
	uintptr_t nextRTDepth;
	uintptr_t nextSPP;
	uintptr_t nextMemProtActive;
	
	uint8_t *prevImg;
	uint8_t *drawImg;
	uint8_t *decompBuffer;

	QList<ImgData*> imgDeque;
	sem_t sem;
	pthread_mutex_t dataLock;
	pthread_mutex_t guiLock;

	void signal();
	void wait();

	void lock(pthread_mutex_t *mutex);
	void unlock(pthread_mutex_t *mutex);

	void init();
	void doWork();
	void run();
};

#endif /* DEMO_SOCKET_WORKER */
