#ifndef DEMO_AABB_HEADER
#define DEMO_AABB_HEADER

#include <inttypes.h>
#include "dmath.h"
#include "ray.h"

class AABB
{
	public:
	Vec minC;
	Vec maxC;

	Vec get(uintptr_t idx) const
	{
		if(idx == 0) return minC;
		return maxC;
	}

	void reset()
	{
		minC.x = REAL_MAX;
		minC.y = REAL_MAX;
		minC.z = REAL_MAX;

		maxC.x = REAL_MIN;
		maxC.y = REAL_MIN;
		maxC.z = REAL_MIN;
	}

	AABB()
	{
		reset();
	}

	void add(Vec v)
	{
		if(v.x < minC.x) minC.x = v.x;
		if(v.y < minC.y) minC.y = v.y;
		if(v.z < minC.z) minC.z = v.z;

		if(v.x > maxC.x) maxC.x = v.x;
		if(v.y > maxC.y) maxC.y = v.y;
		if(v.z > maxC.z) maxC.z = v.z;
	}

	void add(const AABB *aabb)
	{
		add(aabb->minC);
		add(aabb->maxC);
	}

	bool isInit()
	{
		return maxC.x >= minC.x;
	}

	real getSurfaceArea() 
	{
		if(!isInit())
		{
			return real(0);
		}

		Vec lengths = maxC - minC;
		return real(2) * (lengths.x * lengths.y + lengths.y * lengths.z + lengths.x * lengths.z);
	}

	Vec getMidpoint()
	{
		return (minC + maxC) * real(0.5);
	}

	uintptr_t maxAxis() const
	{
		Vec delta = maxC-minC;
		if(delta.x > delta.y)
			return (delta.x > delta.z ? 0 : 2);
		else
			return (delta.y > delta.z ? 1 : 2);
	}

	bool intersect(const Ray *ray, const Vec invDir, const bool dirIsNeg[3]) const
	{
		real tmin  = (get( dirIsNeg[0]).x - ray->origin.x) * invDir.x;
		real tmax  = (get(!dirIsNeg[0]).x - ray->origin.x) * invDir.x;
		real tymin = (get( dirIsNeg[1]).y - ray->origin.y) * invDir.y;
		real tymax = (get(!dirIsNeg[1]).y - ray->origin.y) * invDir.y;

		if ((tmin > tymax) || (tymin > tmax))
			return false;
		if (tymin > tmin) tmin = tymin;
		if (tymax < tmax) tmax = tymax;

		real tzmin = (get( dirIsNeg[2]).z - ray->origin.z) * invDir.z;
		real tzmax = (get(!dirIsNeg[2]).z - ray->origin.z) * invDir.z;

		if ((tmin > tzmax) || (tzmin > tmax))
			return false;
		return true;
		//if (tzmin > tmin)
		//	tmin = tzmin;
		//if (tzmax < tmax)
		//	tmax = tzmax;

		//return (tmin < ray->maxt) && (tmax > ray->mint);
	}

	bool intersect(const Ray *ray) const
	{
		Vec invDir(real(1.0) / ray->dir.x, real(1.0) / ray->dir.y, real(1.0) / ray->dir.z);
		bool sign[3] = {
			invDir.x < real(0.0),
			invDir.y < real(0.0),
			invDir.z < real(0.0)
		};
		return intersect(ray, invDir, sign);
	}
};

#endif /* DEMO_AABB_HEADER */
