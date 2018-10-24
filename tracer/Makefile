.PHONY: all clean

all: linux

linux: compressor.cc demo.cc intersector.cc invasic.cc network.cc scene.cc sphere.cc ../lzo/minilzo.c aabb.h camera.h compressor.h dmath.h intersector.h material.h network.h ray.h scene.h sphere.h texture.h util.h Makefile
	g++ -std=c++11 -Wall -Wextra -fopenmp -Ofast -flto -o linux compressor.cc demo.cc intersector.cc invasic.cc network.cc scene.cc sphere.cc ../lzo/minilzo.c -lm

clean:
	rm -f linux

