#include "sphere.h"
#include "aabb.h"

real Sphere::intersect(const Ray *ray) const
{
	Vec p = mid - ray->origin;

	real b = p * ray->dir;
	real det = b * b - (p * p) + radius * radius;

	if(det < real(0)) {
		return REAL_MAX;
	}

	det = Math::sqrt(det);
	real i2 = b + det;

	if(i2 < EPSILON) {
		return REAL_MAX;
	}

	real outt = i2;
	real i1 = b - det;
	if(i1 > EPSILON) {
		outt = i1;
	}

	return outt;
}

void Sphere::getAABB(AABB *aabb) const
{
	Vec radvec(radius, radius, radius);
	aabb->add(mid+radvec);
	aabb->add(mid-radvec);
}
