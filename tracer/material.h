#ifndef DEMO_MATERIAL_HEADER
#define DEMO_MATERIAL_HEADER

#include "dmath.h"
#include "texture.h"

class Material
{
	public:
	enum MaterialType {
		GLASS = 0,
		MIRROR = 1,
		DIFFUSE = 2,
	};

	Vec color;
	Texture *texture;
	MaterialType type;
	bool rotate;

	Material()
	{
		texture = nullptr;
		type = DIFFUSE;
		rotate = false;
	}

	Vec getColor(Vec normal, real globTime)
	{
		if(texture == nullptr) {
			return color;
		}

		if(rotate) {
			real angle = globTime * 3;
			real sinangle;
			real cosangle;
			Math::sincos(angle, &sinangle, &cosangle);
			Vec tmp(
				normal.x * cosangle + normal.z * sinangle,
				normal.y,
				normal.z * cosangle - normal.x * sinangle
			);

			real u = tmp.x * 0.5 + 0.5;
			real v = tmp.y * 0.5 + 0.5;
			if(tmp.z < real(0.0)) {
				u = real(1.0) - u;
			}
			return texture->getColor(u, v, color);
		}
		else {
			real u = normal.x * 0.5 + 0.5;
			real v = normal.y * 0.5 + 0.5;
			if(normal.z < real(0.0)) {
				u = real(1.0) - u;
			}
			return texture->getColor(u, v, color);
		}

	}
};

#endif /* DEMO_MATERIAL_HEADER */
