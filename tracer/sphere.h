#ifndef DEMO_SPHERE_HEADER
#define DEMO_SPHERE_HEADER

#include "dmath.h"
#include "material.h"
#include "ray.h"

class AABB;

class Sphere
{
	public:
	Vec mid;
	real radius;
	real radiusInv;

	Material material;

	void init(const Vec pos, real rad)
	{
		mid = pos;
		radius = rad;
		radiusInv = real(1) / radius;
	}

	Sphere()
	{
		init(Vec(0,0,0), 1);
	}

	Sphere(const Vec pos, real rad)
	{
		init(pos, rad);
	}

	real intersect(const Ray *ray) const;

	void getAABB(AABB *aabb) const;
};

#endif /* DEMO_SPHERE_HEADER */
