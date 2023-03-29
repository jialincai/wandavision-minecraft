#ifndef PROCEDURALTERRAINHELP_H
#define PROCEDURALTERRAINHELP_H

#include "glm_includes.h"

using namespace glm;

/* Generate a pseudo-random float. */
float random1(vec2 p);
float random1(float n);

/* Generate a pseudo-random vec2. */
glm::vec2 random2(vec2 p);

/* Generate a pseudo-random vec3. */
glm::vec3 random3(vec3 p);

/* Remaps value from [low1, hi1] to [low2, hi2]. */
float remap(float val, float low1, float hi1,
                       float low2, float hi2);
//
// Fractal Brownian Motion
float interpNoise1D (float x);
float interpNoise2D(vec2 p);
float fbm1D(float n, uint octaves);
float fbm2D(vec2 p, uint octaves);
//
// Worley
/* Note this worley noise function is based on the distance
   of TWO closest points. Yields a "pointy" worly noise. */
float worley(vec2 p);
//
// Perlin
float perlin(vec2 p);
float surflet(vec2 p, vec2 gridPoint);
float perlin3d(vec3 p);
float surflet3d(vec3 p, vec3 gridPoint);

#endif // PROCEDURALTERRAINHELP_H
