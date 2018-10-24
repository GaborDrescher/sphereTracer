#ifndef DEMO_UTIL_HEADER
#define DEMO_UTIL_HEADER

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __OCTOPOS__
#	include "octopos.h"
#else
#	include <time.h>
#endif

#define malloc_or_die(size) Util::mallocOrDie(size)

class Util
{
	private:
	static void archAbort()
	{
		#ifdef __OCTOPOS__
			abort();
		#else
			exit(-1);
		#endif
	}
	public:
	enum {
		COMPRESSED_PICTURE = 0,
		STATISTICS = 1
	};

	static void* mallocOrDie(uintptr_t size)
	{
		void *out = malloc(size);
		if(out == NULL) {
			fprintf(stderr, "malloc(%" PRIuPTR ") failed\n", size);
			archAbort();
		}
		return out;
	}
	
	static uint64_t my_timer_start()
	{
		#ifdef __OCTOPOS__
			return timer_start();
		#else
			struct timespec tp;
			tp.tv_sec = 0;
			tp.tv_nsec = 0;
			clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
			return (tp.tv_sec * ((uint64_t)1000000000)) + (uint64_t)tp.tv_nsec;
		#endif
	}
	
	static uint64_t my_timer_stop()
	{
		#ifdef __OCTOPOS__
			return timer_stop();
		#else
			return my_timer_start();
		#endif
	}
	
	static uint64_t ticks_to_ns(uint64_t ticks)
	{
		#ifdef __OCTOPOS__
			return ticks_to_nanoseconds(ticks);
		#else
			return ticks;
		#endif
	}

	static void die(const char *msg = NULL)
	{
		if(msg == NULL || msg[0] == '\0') {
			fprintf(stderr, "ERROR\n");
		}
		else {
			fprintf(stderr, "ERROR: %s\n", msg);
		}
		archAbort();
	}

	static void perror_die(const char *msg)
	{
		#ifdef __OCTOPOS__
			die(msg);
		#else
			perror(msg);
			archAbort();
		#endif
	}

	static void setMemProt(bool activate)
	{
		if(activate) {
			printf("activating memory protection\n");
			#ifdef __OCTOPOS__
			octo_app_protect();
			#endif
		}
		else {
			printf("deactivating memory protection\n");
			#ifdef __OCTOPOS__
			octo_app_unprotect();
			#endif
		}
	}
};

#ifndef __OCTOPOS__
namespace hw {
namespace hal {
	class Atomic
	{
		public:
		template<typename T>
		static T load(volatile T *mem)
		{
			return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
		}

		template<typename T>
		static void store(volatile T *mem, T val)
		{
			__atomic_store_n(mem, val, __ATOMIC_SEQ_CST);
		}

		template<typename T>
		static T fetchAdd(volatile T *mem, T val)
		{
			return __atomic_fetch_add(mem, val, __ATOMIC_SEQ_CST);
		}
		static void busy()
		{
		}
	};
} // namespace hal
} // namespace hw

typedef uint16_t claim_t;

#endif

#endif /* DEMO_UTIL_HEADER */
