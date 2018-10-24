#ifndef DEMO_RAY_HEADER
#define DEMO_RAY_HEADER

#include "dmath.h"

class Ray
{
	public:
	Vec origin;
	Vec dir;
	real mint;
	real maxt;

	Ray()
	{
	}

	Ray(const Vec o, const Vec d)
	{
		origin = o;
		dir = d;
		mint = EPSILON;
		maxt = REAL_MAX;
	}

	Vec getPoint(real dist)
	{
		return origin + (dir * dist);
	}
};

#endif /* DEMO_RAY_HEADER */
