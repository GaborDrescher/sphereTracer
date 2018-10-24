#ifndef DEMO_TEXTURE_HEADER
#define DEMO_TEXTURE_HEADER

#include "dmath.h"

class Texture
{
	private:
	uintptr_t width;
	uintptr_t height;
	real widthSub1;
	real heightSub1;

	const uint8_t *rgbaData;
	bool nearest;

	Vec getIntColor(uintptr_t x, uintptr_t y, Vec matColor)
	{
		if(x >= width) {
			x = width - 1;
		}
		if(y >= height) {
			y = height - 1;
		}
	
		uintptr_t idx = (x + y * width) * 4;
		real r = rgbaData[idx + 0];
		real g = rgbaData[idx + 1];
		real b = rgbaData[idx + 2];
		real a = rgbaData[idx + 3];

		real norm = real(1.0) / real(255.0);

		Vec out;
		out.x      = r * norm;
		out.y      = g * norm;
		out.z      = b * norm;
		real alpha = a * norm;
	
		out = (out * alpha) + (matColor * (real(1.0) - alpha));
	
		return out;
	}

	public:
	void init(uintptr_t width, uintptr_t height, const uint8_t *data, bool nearest=false)
	{
		this->width = width;
		this->height = height;
		this->widthSub1 = real(width - 1);
		this->heightSub1 = real(height - 1);

		this->rgbaData = data;
		this->nearest = nearest;
	}

	Vec getColor(real u, real v, Vec matColor)
	{
		real x = widthSub1 * u;
		real y = heightSub1 * v;
	
		uintptr_t intX = x;
		uintptr_t intY = y;

		if(nearest) {
			return getIntColor(intX, intY, matColor);
		}

		// else bilinear interpolation

		real remX = x - ((uintptr_t)x);
		real remY = y - ((uintptr_t)y);
		real remXinv = real(1.0) - remX;
		real remYinv = real(1.0) - remY;
	
		Vec leftUp = getIntColor(intX, intY, matColor);
		Vec rightUp = getIntColor(intX+1, intY, matColor);
		Vec leftDown = getIntColor(intX, intY+1, matColor);
		Vec rightDown = getIntColor(intX+1, intY+1, matColor);
	
		Vec a = (leftUp * remXinv) + (rightUp * remX);
		Vec b = (leftDown * remXinv) + (rightDown * remX);
	
		return (a * remYinv) + (b * remY);
	}
};


#endif /* DEMO_TEXTURE_HEADER */
