#include "intersector.h"
#include "dmath.h"
#include "sphere.h"

Intersector::Intersector()
{
	this->spheres = nullptr;
	this->nSpheres = 0;
}

void Intersector::init(Sphere *spheres, uintptr_t nSpheres)
{
	this->spheres = spheres;
	this->nSpheres = nSpheres;

	//aabb.reset();

	//for(uintptr_t i = 0; i < nSpheres; ++i) {
	//	spheres[i].getAABB(&aabb);
	//}
}

Sphere* Intersector::intersect(const Ray *ray, real *outDist) const
{
	Sphere *nearest = nullptr;
	real dist = REAL_MAX;

	//if(aabb.intersect(ray)) {
		// find nearest hit
		for(uintptr_t i = 0; i < nSpheres; ++i) {
			real currDist = spheres[i].intersect(ray);
			if(currDist < dist) {
				dist = currDist;
				nearest = &(spheres[i]);
			}
		}
	//}

	*outDist = dist;
	return nearest;

}
