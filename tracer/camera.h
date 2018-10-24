#ifndef DEMO_CAMERA_HEADER
#define DEMO_CAMERA_HEADER

#include <inttypes.h>
#include "dmath.h"
#include "ray.h"

class Camera
{
	private:
	real fov;
	real near;
	real width;
	real height;
	Vec pos;
	Vec viewDir;
	Vec up;
	Vec left;
	Vec corner;

	public:
	void init(Vec pos_, Vec lookat_, Vec up_, real fovDeg_, uintptr_t w_, uintptr_t h_)
	{
		//pos_ = vec(pos_.x, pos_.z, pos_.y);
		fov = fovDeg_ * (MY_PI / real(180));
		near = real(0.5) / Math::tan(real(0.5) * fov);
		
		width = w_;
		height = h_;
		pos = pos_;

		//base vectors
		viewDir = lookat_ - pos;
		viewDir.normalize();

		up = up_;
		up.normalize();
				
		left = up % viewDir;
		left.normalize();
		
		up = viewDir % left;
		up.normalize();
		
		corner = pos + (viewDir * near) + (left * real(0.5))+ (up * (height/width) * real(0.5));
	}

	Camera(Vec pos_, Vec lookat_, Vec up_, real fovDeg_,  uintptr_t w_, uintptr_t h_)
	{
		init(pos_, lookat_, up_, fovDeg_, w_, h_);
	}
	Camera()
	{
	}

	void getRay(Ray *ray, real x, real y) const
	{
		real xn = x/width;
		real yn = y/height;

		ray->origin = (corner - (left * xn)) - (up * yn);

		Vec dir = ray->origin - pos;
		real dlen = dir.normalize();
		ray->dir = dir;

		ray->mint = dlen;
		ray->maxt = REAL_MAX;
	}
};

#endif /* DEMO_CAMERA_HEADER */
