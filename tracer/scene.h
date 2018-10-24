#ifndef DEMO_SCENE_HEADER
#define DEMO_SCENE_HEADER

#include <inttypes.h>
#include <stdlib.h>
#include "sphere.h"
#include "camera.h"
#include "ray.h"
#include "intersector.h"

class Scene
{
	public:
	Intersector intersector;
	uintptr_t width;
	uintptr_t height;
	uintptr_t rtDepth;
	uintptr_t spp;

	Sphere *spheres;
	uintptr_t size;
	real sphereRadius;

	Camera cam;
	Vec lightPos;
	Vec lightColor;
	real globTime;

	Ray *precompRays;

	void init()
	{
		width = 0;
		height = 0;
		rtDepth = 0;
		spp = 0;
		spheres = nullptr;
		size = 0;
		sphereRadius = 0;
		precompRays = nullptr;
		globTime = 0;
	}

	void init(uintptr_t nspheres, uintptr_t sphereRad, uintptr_t width, uintptr_t height, uintptr_t rtdepth, uintptr_t spp, Vec pos);

	private:
	Sphere* intersect(const Ray *ray, real *outDist) const;

	Vec bgColor(const Ray &ray) const;

	public:
	Vec render(const Ray &ray, uintptr_t depth = 0) const;

	Vec render(uintptr_t x, uintptr_t y) const;

	void advance(real dt);
};

#endif /* DEMO_SCENE_HEADER */
