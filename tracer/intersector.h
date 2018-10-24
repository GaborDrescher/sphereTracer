#ifndef DEMO_INTERSECTOR_HEADER
#define DEMO_INTERSECTOR_HEADER

#include "dmath.h"
#include "aabb.h"

class Sphere;
class Ray;

class Intersector
{
	private:
	Sphere *spheres;
	uintptr_t nSpheres;
	AABB aabb;

	public:
	Intersector();
	void init(Sphere *spheres, uintptr_t nSpheres);
	Sphere* intersect(const Ray *ray, real *outDist) const;
};


#endif /* DEMO_INTERSECTOR_HEADER */
