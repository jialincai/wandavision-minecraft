#include "proceduralterrainhelp.h"

float random1(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453123);
}

float random1 (float n) {
    return fract(sin(n * 127.1) * 43758.5453123);
}

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p,vec2(127.1, 311.7)),
                          dot(p,vec2(269.5, 183.3)))) * 43758.5453f);
}

vec3 random3(vec3 p) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 489.61)),
                          dot(p,vec3(777.7, 444.4, 333.3)),
                          dot(p,vec3(269.5, 183.3, 914.5)))) * 43758.5453f);
}

float remap (float val, float low1, float hi1,
                                 float low2, float hi2) {
    return (val - low1) / (hi1 - low1) * (hi2 - low2) + low2;
}

float interpNoise1D (float x) {
    int intX = int(floor(x));
    float fractX = fract(x);

    float v1 = random1(intX);
    float v2 = random1(intX + 1);
    return mix(v1, v2, fractX);
}

float interpNoise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = random1(i);
    float b = random1(i + vec2(1.0, 0.0));
    float c = random1(i + vec2(0.0, 1.0));
    float d = random1(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0f - 2.0f * f);

    float i1 = mix(a, b, u.x);
    float i2 = mix(c, d, u.x);
    return mix(i1, i2, u.y);
}

float fbm1D(float n, uint octaves) {
    float value = 0;
    float persistence = 0.5f;
    float amp = 0.5f;
    float growth_factor = 2.0;
    //
    for(uint i = 1; i <= octaves; i++) {
        value += interpNoise1D(n) * amp;

        n *= growth_factor;
        amp *= persistence;
    }
    return value;

}

float fbm2D(vec2 p, uint octaves) {
    // Initial values
    float value = 0.0;
    float persistence = 0.5;
    float amp = .5;
    float growth_factor = 3.0;
    //
    // Loop of octaves
    for (uint i = 0; i < octaves; i++) {
        value += amp * interpNoise2D(p);

        p *= growth_factor;
        amp *= persistence;
    }
    return value;
}

float worley(vec2 p) {
    vec2 uvInt = floor(p);
    vec2 uvFract = fract(p);

    float minDist = 1.0;
    float secondMinDist = 1.0;
    vec2 closestPoint;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = random2(uvInt + neighbor);

            // Computer the distance between the point and the fragment.
            // Store the min dist so far
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);

            if (dist < minDist) {
                secondMinDist = minDist;
                minDist = dist;
                closestPoint = point;
            } else if (dist < secondMinDist) {
                secondMinDist = dist;
            }
        }
    }
    float height = -1 * minDist + 1 * secondMinDist;    // Change scalers to return different flavors of worley noise.
    return height;
}

float quinticFalloff(float f) {
    return 1 - 6 * pow(f, 5.f) + 15 * pow(f, 4.f) - 10 * pow(f, 3.f);
}

float perlin(vec2 uv) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(uv, floor(uv) + vec2(dx, dy));
        }
    }
    return surfletSum;

}

float surflet(vec2 P, vec2 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = quinticFalloff(distX);
    float tY = quinticFalloff(distY);
    // Get the random vector for the grid point
    vec2 gradient = 2.f * random2(gridPoint) - vec2(1.f);
    // Get the vector from the grid point to P
    vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;

}

float surflet3d(vec3 p, vec3 gridPoint) {
    vec3 dist = abs(p - gridPoint); //distance b/w the corner(grid) point and the point in the grid under consideration
    vec3 falloff = vec3(quinticFalloff(dist.x), quinticFalloff(dist.y), quinticFalloff(dist.z)); //quintic falloff to smoothen out the cells
    // Get the random vector for the grid point (assume we wrote a function random2
    // that returns a vec2 in the range [0, 1])
    vec3 gradient = normalize(2.f * random3(gridPoint) - vec3(1., 1., 1.));
    // Get the vector from the grid point to P
    vec3 diff = p - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * falloff.x * falloff.y * falloff.z;
}

float perlin3d(vec3 p) {
    float surfletSum = 0.f;
    // Iterate over the eight integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; dz++) {
                surfletSum += surflet3d(p, floor(p) + vec3(dx, dy, dz));
            }
        }
    }
    return surfletSum;
}

