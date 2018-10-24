#include "scene.h"
#include "util.h"
#include "texture.h"

extern uintptr_t mat_invasic_width;
extern uintptr_t mat_invasic_height;
extern const char *mat_invasic_data;

void Scene::init(uintptr_t nspheres, uintptr_t sphereRad, uintptr_t width, uintptr_t height, uintptr_t rtdepth, uintptr_t spp, Vec pos)
{
	// clean up old state
	free(precompRays);
	precompRays = nullptr;

	free(spheres);
	spheres = nullptr;

	this->width = width;
	this->height = height;
	this->rtDepth = rtdepth;
	this->spp = spp;

	this->spheres = (Sphere*)malloc_or_die(sizeof(Sphere) * nspheres);
	this->sphereRadius = real(sphereRad) * real(0.1);
	this->size = nspheres;
	this->cam.init(pos, Vec(0, 0, 0), Vec(0, 1, 0), real(39.5), width, height);

	Texture *invasicTexture = (Texture*)Util::malloc_or_die(sizeof(Texture));
	invasicTexture->init(mat_invasic_width, mat_invasic_height, (const uint8_t*)mat_invasic_data);

	for(uintptr_t i = 0; i < nspheres; ++i) {
		if((i % 2) == 0) {
			spheres[i].init(Vec(0, 0, 0), sphereRadius);
			spheres[i].material.color = Vec(0.0, 0.2, 0.5);
			spheres[i].material.texture = nullptr;
			spheres[i].material.type = Material::DIFFUSE;
			if((i % 4) == 0) {
				spheres[i].material.color = Vec(0.9, 0.9, 0.9);
				spheres[i].material.texture = invasicTexture;
				spheres[i].material.rotate = true;
				spheres[i].material.type = Material::DIFFUSE;
			}
		}
		else {
			spheres[i].init(Vec(0, 0, 0), sphereRadius);
			spheres[i].material.color = Vec(0.9, 0.9, 0.9);
			spheres[i].material.texture = nullptr;
			spheres[i].material.type = Material::MIRROR;
		}
	}

	this->lightPos = Vec(100, -100, 0);
	this->lightColor = Vec(1, 1, 1);

	this->intersector.init(this->spheres, this->size);

	precompRays = NULL;
	if(spp != 4) {
		// try to store precomputed rays for all pixels, if allocation fails for
		// this just compute them on the fly
		precompRays = (Ray*)malloc(sizeof(Ray) * width * height);
		if(precompRays != NULL) {
			for(uintptr_t y = 0; y < height; ++y) {
				for(uintptr_t x = 0; x < width; ++x) {
					cam.getRay(&(precompRays[x + y * width]), x, y);
				}
			}
		}
	}
}

Vec Scene::bgColor(const Ray &ray) const
{
	//checkerboard texture
	const real numSquaresPerCubeSide(5 * 0.5);

	Vec cubeLoc = ray.dir;
	cubeLoc = (cubeLoc + Vec(1.0, 1.0, 1.0)) * numSquaresPerCubeSide;
	int cubeLocx = ((int)cubeLoc.x) & 1;
	int cubeLocy = ((int)cubeLoc.y) & 1;
	int cubeLocz = ((int)cubeLoc.z) & 1;
	real sum = (cubeLocx + cubeLocy + cubeLocz) & 1;
	real checker = sum * real(0.1) + real(0.9);

	// color
	real val = (ray.dir.x + real(1.0)) * real(0.5);
	Vec right(0.5, 0.2, 0.0);
	Vec left(0.0, 0.2, 0.5);
	Vec color = right * val + left * (real(1.0) - val);

	return color * checker;
}

Vec Scene::render(const Ray &ray, uintptr_t depth) const
{
	if(depth >= rtDepth) {
		return Vec(0,0,0);
	}

	real dist;
	Sphere *nearest = intersector.intersect(&ray, &dist);
	if(nearest == nullptr) {
		return bgColor(ray);
	}

	const Vec hitpoint = ray.origin + (ray.dir * dist);
	const Vec normal = (hitpoint - nearest->mid) * nearest->radiusInv;

	Material::MaterialType type = nearest->material.type;
	Vec matColor = nearest->material.getColor(normal, globTime);

	const real shininess = 128.0;
	Vec lightDir = lightPos - hitpoint;
	lightDir.normalize();
	real lambertian = normal * lightDir;

	Vec ambientColor = matColor;
	Vec diffuseColor;
	Vec specularColor;
	if(lambertian > real(0)) {
		diffuseColor = matColor * lambertian;

		// blinn phong shininess
		Vec halfDir = lightDir - ray.dir;
		halfDir.normalize();
		real specAngle = Math::max(halfDir * normal, real(0));
		if(specAngle > real(0)) {
			real specular = Math::pow(specAngle, shininess);
			specularColor = lightColor * specular;
		}
	}
	Vec blinnPhongColor = ambientColor + diffuseColor + specularColor;

	if(type == Material::DIFFUSE) {
		matColor = blinnPhongColor;
	}
	else if(type == Material::MIRROR) {
		Ray outRay;
		outRay.dir = ray.dir - normal * (real(2) * (normal * ray.dir));
		outRay.origin = hitpoint + (outRay.dir * real(0.05));

		Vec recColor = render(outRay, depth + 1);
		matColor = matColor.scale(recColor);

		matColor = matColor.scale(blinnPhongColor);
	}
	else { // GLASS
		Vec nl = (normal * ray.dir) < 0 ? normal : (normal * real(-1));
		Ray reflRay(hitpoint, ray.dir - normal * real(2) * (normal * ray.dir));
		reflRay.origin = reflRay.origin + (reflRay.dir * real(0.05));
		reflRay.dir.normalize();

		bool into = (normal * nl) > 0;                // Ray from outside going in?

		real nc=1;
		real nt=1.5;
		real nnt = into ? (nc/nt) : (nt/nc);
		real ddn = ray.dir * nl;
		real cos2t = real(1) - nnt * nnt * (real(1) - ddn * ddn);

		if(cos2t < real(0)) {    // Total internal reflection 
			return matColor.scale(render(reflRay, depth+1));
		}

		Vec tdir = ray.dir * nnt - normal * ((into ? real(1) : real(-1)) * (ddn * nnt + Math::sqrt(cos2t)));
		tdir.normalize();

		real a = nt-nc;
		real b=nt+nc;
		real R0=a*a/(b*b);
		real c = real(1) - (into ? -ddn : (tdir * normal));
		real Re = R0 + (real(1) - R0) * c*c*c*c*c;
		real Tr = real(1) - Re;
		
		Ray mirrRay(hitpoint, tdir);
		mirrRay.origin = mirrRay.origin + (mirrRay.dir * real(0.05));

		matColor = matColor.scale(render(reflRay,depth + 1) * Re + render(mirrRay, depth + 1) * Tr);
		matColor = matColor.scale(blinnPhongColor);
	}
	return matColor;
}

Vec Scene::render(uintptr_t x, uintptr_t y) const
{
	if(spp != 4) {
		Ray ray;
		if(precompRays != NULL) {
			ray = precompRays[x + y * width];
		}
		else {
			cam.getRay(&ray, x, y);
		}
		return render(ray);
	}

	// 4x SSAA
	real rx = x;
	real ry = y;
	//rx += real(0.25);
	//ry += real(0.25);

	Ray r0, r1, r2 ,r3;
	cam.getRay(&r0, rx, ry);
	cam.getRay(&r1, rx+real(0.5), ry);
	cam.getRay(&r2, rx, ry+real(0.5));
	cam.getRay(&r3, rx+real(0.5), ry+real(0.5));

	Vec c0, c1, c2, c3;
	c0 = render(r0) * real(0.25);
	c1 = render(r1) * real(0.25);
	c2 = render(r2) * real(0.25);
	c3 = render(r3) * real(0.25);

	return c0+c1+c2+c3;
}

void Scene::advance(real dt)
{
	globTime += dt;
	if(globTime > real(360)) {
		globTime -= real(360);
	}

	for(uintptr_t i = 0; i < size; ++i) {
		Vec pos;

		//real angle = globTime + ((real)i) / ((real)size) * real(2) * real(MY_PI) + real(MY_PI) * real(0.5);
		real angle = globTime + (((real)i) / (size * real(0.5))) * MY_PI * real(2) + MY_PI * real(0.5);

		//position and radius
		//angle += (((real)(rand() % 1000)) / real(1000)) * real(0.01);
		real si = i < (size/2) ? 1 : -1;
		real ad = i < (size/2) ? 0 : 10;
		pos.x = real(11.0) * Math::cos(angle);
		pos.y = real(3.5) * Math::sin(angle*real(1.3)) * Math::cos(angle*real(1.3)) + ad - real(5);
		pos.z = real(15.0) * Math::sin(angle * si) + real(15);

		spheres[i].mid = pos;
	}

	lightPos.x = real(50.0) * Math::cos(globTime);
	lightPos.y = real(50.0) * Math::sin(globTime);

	this->intersector.init(this->spheres, this->size);
}
