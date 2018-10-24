#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dmath.h"
#include "sphere.h"
#include "scene.h"
#include "camera.h"
#include "ray.h"
#include "material.h"
#include "util.h"
#include "compressor.h"
#include "network.h"

#ifdef __OCTOPOS__
#	include "octopos.h"
#else
#	ifdef _OPENMP
#		include <omp.h>
#	endif
#endif

static_assert(((void*)nullptr) == ((void*)NULL), "");
static_assert(((void*)nullptr) == ((void*)0),    "");

#define swap(a,b) do {auto tmp = a; a = b; b = tmp;} while(false)

#ifndef PAGE_SIZE
#	define PAGE_SIZE (4096)
#endif

#ifdef __OCTOPOS__
	static void noiseFunc(void *unused)
	{
		(void)unused;
	
		const uintptr_t pages = 1;
		printf("starting noise ilet using %" PRIuPTR " pages\n", pages);
	
		for(;;) {
			void *mem = mem_map(MEM_TLM_LOCAL, pages * PAGE_SIZE);
			if(mem == NULL) {
				Util::die("mem_map returned NULL\n");
			}
	
			mem_unmap(mem, pages * PAGE_SIZE);
			hw::hal::Atomic::busy();
		}
	}
	
	static void startNoiseIlet()
	{
		claim_t claim = claim_construct();
		int ret = invade_simple(claim, 1);
		if(ret != 0) {
			Util::die("cannot invade core\n");
		}
	
		simple_ilet ilet;
		simple_ilet_init(&ilet, noiseFunc, 0);
	
		ret = infect(claim, &ilet, 1);
		if(ret != 0) {
			Util::die("cannot infect core\n");
		}
	}
#endif

class Main
{
	private:
	uintptr_t width;
	uintptr_t height;
	uintptr_t rtDepth;
	uintptr_t rgbPicSize;
	uintptr_t measurementVals;
	uintptr_t spheres;
	uintptr_t sphereRadius;
	uintptr_t spp;
	uintptr_t bullshitPadding;

	uint8_t *currentPicture;
	uint8_t *lastPicture;
	uint8_t *diffPicture;

	uint8_t *currCompressOutput;
	uintptr_t currCompressOutputSize;

	uint8_t *lastCompressOutput;
	uintptr_t lastCompressOutputSize;

	uint8_t *inBuffer;

	Network *net;
	Compressor *lzo;
	Scene *scene;

	Vec cameraPosition;

	uintptr_t jitterTime;

	bool memProtActivated;
	bool lastPicZero;

	uintptr_t getInBufferSize() const
	{
		return sizeof(uint64_t) * 7;
	}

	uintptr_t getOutBufferHeaderSize() const
	{
		uintptr_t size =
			  // whole buffer size
			  sizeof(uint64_t)
			  // number of measurement values
			+ sizeof(uint64_t)
			  // actual measurement values
			+ sizeof(uint64_t) * measurementVals
			  // picture width
			+ sizeof(uint64_t)
			  // picture height
			+ sizeof(uint64_t)
			  // whether 'lastPicture' was zero
			+ sizeof(uint64_t)
			  // compressed picture size
			+ sizeof(uint64_t)
			;
		return size;
	}

	uintptr_t getOutBufferPicSize() const
	{
		// compressed picture data, worst case size
		return (rgbPicSize + rgbPicSize / 16 + 64 + 3);
	}

	uintptr_t getOutBufferSize() const
	{
		uintptr_t encodedDataSize = getOutBufferHeaderSize() + getOutBufferPicSize();
		if(encodedDataSize < bullshitPadding) {
			encodedDataSize = bullshitPadding * 2;
		}
		return encodedDataSize;
	}

	static void encode64(uint8_t *out, uint64_t val)
	{
		for(uintptr_t i = 0; i < 8; ++i) {
			out[i] = (uint8_t)(val >> (8 * i));
		}
	}

	static uint64_t decode64(uint8_t *in)
	{
		uint64_t val = 0;
		for(uintptr_t i = 0; i < 8; ++i) {
			val |= ((uint64_t)(in[i])) << (i*8);
		}
		return val;
	}

	void writeStat(uintptr_t idx, uint64_t val)
	{
		if(idx >= measurementVals) {
			Util::die("too many measurements");
		}

		encode64(currCompressOutput + (2 * sizeof(uint64_t)) + idx * sizeof(uint64_t), val);
	}

	uintptr_t finishOutputBuffer(uintptr_t comprSize)
	{
		uintptr_t encodedDataSize = getOutBufferHeaderSize() + comprSize;
		if(encodedDataSize < bullshitPadding) {
			encodedDataSize = bullshitPadding;
		}

		encode64(currCompressOutput + sizeof(uint64_t) * 0, encodedDataSize);
		encode64(currCompressOutput + sizeof(uint64_t) * 1, measurementVals);
		encode64(currCompressOutput + sizeof(uint64_t) * (measurementVals + 2), width);
		encode64(currCompressOutput + sizeof(uint64_t) * (measurementVals + 3), height);
		encode64(currCompressOutput + sizeof(uint64_t) * (measurementVals + 4), lastPicZero ? 1 : 0);
		encode64(currCompressOutput + sizeof(uint64_t) * (measurementVals + 5), comprSize);

		return sizeof(uint64_t) + encodedDataSize;
	}

	claim_t *claims;
	uintptr_t nClaims;
	uintptr_t workerLine;

	public:
	bool renderLine(uintptr_t y, uintptr_t id)
	{
		(void)id;

		if(y >= height) {
			return false;
		}

		for(uintptr_t x = 0; x < width; ++x) {
			uintptr_t idx = (x + y * width) * 3;
			if(y == 0) {
				jitterTime = Util::my_timer_start();
			}
			Vec color = scene->render(x, y);
			if(y == 0) {
				jitterTime = Util::ticks_to_ns(Util::my_timer_stop() - jitterTime);
			}
			currentPicture[idx + 0] = Math::getRGBByte(color.x);
			currentPicture[idx + 1] = Math::getRGBByte(color.y);
			currentPicture[idx + 2] = Math::getRGBByte(color.z);

			diffPicture[idx + 0] = currentPicture[idx + 0] - lastPicture[idx + 0];
			diffPicture[idx + 1] = currentPicture[idx + 1] - lastPicture[idx + 1];
			diffPicture[idx + 2] = currentPicture[idx + 2] - lastPicture[idx + 2];
		}
		return true;
	}

	private:
	static void workerFunc(void *mainptr, void *idptr)
	{
		const uintptr_t id = (uintptr_t)idptr;
		Main *myMain = (Main*)mainptr;

		for(;;) {
			uintptr_t line = hw::hal::Atomic::fetchAdd(&myMain->workerLine, (uintptr_t)1);
			bool success = myMain->renderLine(line, id);
			if(!success) {
				break;
			}
		}
	}

	void initWorkers()
	{
		claims = nullptr;
		nClaims = 1;
		workerLine = 0;

		#ifdef __OCTOPOS__
		for(;;) {
			claim_t claim = claim_construct();
			int ret = invade_simple(claim, 1);
			if(ret != 0) {
				break;
			}
			nClaims += 1;

			claims = (claim_t*)realloc(claims, sizeof(claim_t) * (nClaims - 1));
			if(claims == nullptr) {
				fprintf(stderr, "realloc(%p, %" PRIuPTR ") failed\n", claims, sizeof(claim_t) * nClaims);
				Util::die();
			}

			claims[nClaims - 2] = claim;
		}
		#else
			#ifdef _OPENMP
			nClaims = omp_get_max_threads();
			#endif
		#endif
		printf("using %" PRIuPTR " cores for rendering\n", nClaims);
	}

	void startWork()
	{
		hw::hal::Atomic::store(&workerLine, (uintptr_t)0);

		#ifdef __OCTOPOS__
			for(uintptr_t i = 0; i < (nClaims-1); ++i) {
				simple_ilet ilet;
				dual_ilet_init(&ilet, workerFunc, (void*)this, (void*)i);

				int ret = infect(claims[i], &ilet, 1);
				if(ret != 0) {
					Util::die("could not infect claim\n");
				}
			}
			workerFunc(this, (void*)(nClaims-1));
		#else
			#pragma omp parallel
			#pragma omp single nowait
			{
				for(uintptr_t i = 0; i < nClaims; ++i) {
					#pragma omp task
					workerFunc(this, (void*)i);
				}
			}
		#endif

		const uintptr_t maxLineVal = height + nClaims;
		for(;;) {
			uintptr_t currentLine = hw::hal::Atomic::load(&workerLine);
			if(currentLine == maxLineVal) {
				break;
			}
			hw::hal::Atomic::busy();
		}
	}

	public:
	Main()
	{
		width = 0;
		height = 0;
		rtDepth = 0;
		rgbPicSize = 0;
		measurementVals = 0;
		spheres = 0;
		sphereRadius = 0;
		spp = 0;
		bullshitPadding = 0;

		currentPicture = nullptr;
		lastPicture = nullptr;
		diffPicture = nullptr;
		currCompressOutput = nullptr;
		currCompressOutputSize = 0;
		lastCompressOutput = nullptr;
		lastCompressOutputSize = 0;

		net = (Network*)malloc_or_die(sizeof(Network));
		net->init();

		lzo = (Compressor*)malloc_or_die(sizeof(Compressor));
		lzo->init();

		scene = (Scene*)malloc_or_die(sizeof(Scene));
		scene->init();

		cameraPosition = Vec(0,0,60);
		
		inBuffer = (uint8_t*)malloc_or_die(getInBufferSize());

		jitterTime = 0;

		memProtActivated = true;
		lastPicZero = true;
	}

	void init(uintptr_t w, uintptr_t h, uintptr_t rtdepth, uintptr_t nSpheres, uintptr_t sphereRad, uintptr_t samplesPerPixel, Vec camPos)
	{
		width = w;
		height = h;
		rtDepth = rtdepth;
		rgbPicSize = width * height * 3;
		measurementVals = 3;
		spheres = nSpheres;
		sphereRadius = sphereRad;
		spp = samplesPerPixel;

		free(lastCompressOutput);
		free(currCompressOutput);
		free(diffPicture);
		free(lastPicture);
		free(currentPicture);
	
		currentPicture     = (uint8_t*)malloc_or_die(rgbPicSize);
		
		lastPicture        = (uint8_t*)malloc_or_die(rgbPicSize);
		memset(lastPicture, 0, rgbPicSize);

		diffPicture        = (uint8_t*)malloc_or_die(rgbPicSize);
		currCompressOutput = (uint8_t*)malloc_or_die(getOutBufferSize());
		memset(currCompressOutput, 0, getOutBufferSize());

		lastCompressOutput = (uint8_t*)malloc_or_die(getOutBufferSize());
		memset(lastCompressOutput, 0, getOutBufferSize());

		currCompressOutputSize = 0;
		lastCompressOutputSize = 8;

		cameraPosition = camPos;

		scene->init(spheres, sphereRadius, width, height, rtDepth, spp, cameraPosition);
		lastPicZero = true;
	}

	int run(int argc, char *argv[])
	{
		uintptr_t widthIn   = 800;
		uintptr_t heightIn  = 800;
		uintptr_t rtdepth   = 4;
		uintptr_t spheresIn =  12;
		uintptr_t sphereRad =  25;

		if(argc >= 3) {
			widthIn = atoi(argv[1]);
			heightIn = atoi(argv[2]);
			if(widthIn <= 0) {
				widthIn = 10;
			}
			if(heightIn <= 0) {
				heightIn = 10;
			}
		}
		if(argc >= 4) {
			spheresIn = atoi(argv[3]);
			if(spheresIn <= 0) {
				spheresIn = 2;
			}
		}

		init(widthIn, heightIn, rtdepth, spheresIn, sphereRad, 1, Vec(0,0,60));

		initWorkers();

		for(;;) {
			//uint64_t waitTime = Util::my_timer_start();
			net->send(lastCompressOutput, lastCompressOutputSize);
			net->recv(inBuffer, getInBufferSize());
			//waitTime = Util::ticks_to_ns(Util::my_timer_stop() - waitTime);

			//advance simulation
			scene->advance(0.01);
	
			// rendering
			uint64_t time = Util::my_timer_start();

			startWork();
	
			time = Util::my_timer_stop() - time;
			time = Util::ticks_to_ns(time);
			//fprintf(stderr, "rendered %" PRIuPTR " pixels in %" PRIu64 " ms, %" PRIu64 " ns/pixel\n", width * height, time/1000000, time / (width*height));
	
	
			// compression 
			uint64_t comprTime = Util::my_timer_start();
			
			const uintptr_t comprSize = lzo->compress(diffPicture, rgbPicSize, currCompressOutput + getOutBufferHeaderSize(), getOutBufferPicSize());
	
			comprTime = Util::ticks_to_ns(Util::my_timer_stop() - comprTime);
	        //fprintf(stderr, "compressed %" PRIuPTR " bytes into %" PRIuPTR " bytes in %" PRIu64 " ms\n", rgbPicSize, comprSize, comprTime/1000000);


			//// wait for previous network transmissions to finish
			//uint64_t waitTime = Util::my_timer_start();
	
			//net->wait();

			//waitTime = Util::ticks_to_ns(Util::my_timer_stop() - waitTime);
	        ////fprintf(stderr, "waited %" PRIu64 " ms for network\n\n", waitTime/1000000);
	
			// write timing statistics
			writeStat(0, time);
			writeStat(1, jitterTime);
			writeStat(2, comprTime);

			// write final buffer header for currCompressOutput
			currCompressOutputSize = finishOutputBuffer(comprSize);

			// swap current and last pictures/buffers
			swap(currentPicture, lastPicture);
			swap(currCompressOutput, lastCompressOutput);
			swap(currCompressOutputSize, lastCompressOutputSize);
			lastPicZero = false;

			// decode input buffer
			uintptr_t newWidth = decode64(inBuffer + 0*8);
			uintptr_t newHeight = decode64(inBuffer + 1*8);
			uintptr_t newRTDepth = decode64(inBuffer + 2*8);
			uintptr_t newSpheres = decode64(inBuffer + 3*8);
			uintptr_t newSphereRadius = decode64(inBuffer + 4*8);
			uintptr_t newSPP = decode64(inBuffer + 5*8);
			bool newMemProt = decode64(inBuffer + 6*8) != 0;

			if(newWidth != width || newHeight != height || newRTDepth != rtDepth || newSpheres != spheres || newSphereRadius != sphereRadius || newSPP != spp) {
				init(newWidth, newHeight, newRTDepth, newSpheres, newSphereRadius, newSPP, Vec(0,0,60));
			}

			if(newMemProt != memProtActivated) {
				Util::setMemProt(newMemProt);
				memProtActivated = newMemProt;
			}
		}
	
		return 0;
	}

}; // class Main

int main(int argc, char *argv[])
{
	Main prog;
	return prog.run(argc, argv);
}

#ifdef __OCTOPOS__
	#if defined(__cplusplus) || defined(c_plusplus)
	extern "C" {
	#endif
	
	void main_ilet()
	{
		startNoiseIlet();

		char *argv[] = {(char*)"demo", (char*)NULL};
		int ret = main(1, argv);
		exit(ret);
	}
	
	#if defined(__cplusplus) || defined(c_plusplus)
	} /* extern "C" */
	#endif
#endif /* __OCTOPOS__ */
