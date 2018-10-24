#ifndef DEMO_MATH_HEADER
#define DEMO_MATH_HEADER

#include <math.h>
#include <inttypes.h>

#define float_type (0)
#define double_type (1)

//#define real_type float_type
#define real_type double_type

#if real_type == float_type
	typedef float real;
#elif real_type == double_type
	typedef double real;
#else
	#error "real_type not defined"
#endif

#if real_type == float_type
	#define REAL_MAX (real(1e20))
	#define REAL_MIN (real(-1e20))
	#define EPSILON (real(0.001))
#elif real_type == double_type
	#define REAL_MAX (real(1e100))
	#define REAL_MIN (real(-1e100))
	#define EPSILON (real(0.0001))
#else
	#error "real_type not defined"
#endif

#define MY_PI (real(3.14159265358979323846))

class Math
{
	public:
	static real sqrt(real val)
	{
		#if real_type == float_type
			return ::sqrtf(val);
		#elif real_type == double_type
			return ::sqrt(val);
		#endif
	}

	static real tan(real val)
	{
		#if real_type == float_type
			return ::tanf(val);
		#elif real_type == double_type
			return ::tan(val);
		#endif
	}

	static real pow(real x, real y)
	{
		#if real_type == float_type
			return ::powf(x, y);
		#elif real_type == double_type
			return ::pow(x, y);
		#endif
	}

	static real exp(real x)
	{
		#if real_type == float_type
			return ::expf(x);
		#elif real_type == double_type
			return ::exp(x);
		#endif
	}

	static real sin(real x)
	{
		#if real_type == float_type
			return ::sinf(x);
		#elif real_type == double_type
			return ::sin(x);
		#endif
	}

	static real cos(real x)
	{
		#if real_type == float_type
			return ::cosf(x);
		#elif real_type == double_type
			return ::cos(x);
		#endif
	}

	static real round(real x)
	{
		#if real_type == float_type
			return ::roundf(x);
		#elif real_type == double_type
			return ::round(x);
		#endif
	}

	static real fmod(real x, real y)
	{
		#if real_type == float_type
			return ::fmodf(x, y);
		#elif real_type == double_type
			return ::fmod(x, y);
		#endif
	}

	static real atan2(real x, real y)
	{
		#if real_type == float_type
			return ::atan2f(x, y);
		#elif real_type == double_type
			return ::atan2(x, y);
		#endif
	}

	static void sincos(real x, real *sinout, real *cosout)
	{
		#if real_type == float_type
			return ::sincosf(x, sinout, cosout);
		#elif real_type == double_type
			return ::sincos(x, sinout, cosout);
		#endif
	}

	static real clamp(real val, real minVal, real maxVal)
	{
		return val < minVal ? minVal : (val > maxVal ? maxVal : (val));
	}

	static real max(real x, real y)
	{
		return x > y ? x : y;
	}

	static real abs(real x)
	{
		return x < real(0.0) ? -x : x;
	}

	static uint8_t getRGBByte(real val)
	{
		val *= real(255);
		if(val > real(255)) {
			return 255;
		}
		if(val < real(0)) {
			return 0;
		}
		return (uint8_t)val;
	}
};

class Vec
{
	public:
	real x;
	real y;
	real z;

	Vec() : x(0), y(0), z(0)
	{
	}

	Vec(real xIn, real yIn, real zIn) : x(xIn), y(yIn), z(zIn)
	{
	}

	Vec operator+(const Vec other) const
	{
		Vec out(x + other.x, y + other.y, z + other.z);
		return out;
	}

	Vec operator-(const Vec other) const
	{
		Vec out(x - other.x, y - other.y, z - other.z);
		return out;
	}

	Vec operator*(const real val) const
	{
		Vec out(x * val, y * val, z * val);
		return out;
	}

	Vec operator/(const real val) const
	{
		real invVal = real(1) / val;
		return (*this) * invVal;
	}

	real operator*(const Vec other) const
	{
		real out = x * other.x + y * other.y + z * other.z;
		return out;
	}

	Vec scale(const Vec other) const
	{
		Vec out(x * other.x, y * other.y, z * other.z);
		return out;
	}

	Vec operator%(const Vec other) const
	{
		// cross product
		Vec out(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		);
		return out;
	}

	real lengthSq() const
	{
		return x*x + y*y + z*z;
	}

	real length() const
	{
		return Math::sqrt(lengthSq());
	}

	real normalize()
	{
		real len = length();
		real lenInv = real(1) / len;

		x *= lenInv;
		y *= lenInv;
		z *= lenInv;

		return len;
	}
};

#endif /* DEMO_MATH_HEADER */
