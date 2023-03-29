#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec2 fs_UV;

uniform sampler2D u_RenderedTexture;

uniform float u_Time;

out vec4 out_Col;

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p,vec2(127.1, 311.7)),
                          dot(p,vec2(169.5, 983.3)))) * 43758.5453f);
}

float surflet(vec2 p, vec2 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial
    float distX = abs(p.x - gridPoint.x);
    float distY = abs(p.y - gridPoint.y);
    float tX = 1 - 6 * pow(distX, 5.f) + 15 * pow(distX, 4.f) - 10 * pow(distX, 3.f);
    float tY = 1 - 6 * pow(distY, 5.f) + 15 * pow(distY, 4.f) - 10 * pow(distY, 3.f);
    // Get the random vector for the grid point
    vec2 gradient = 2.f * random2(gridPoint) - vec2(1.f);
    // Get the vector from  the grid point to P
    vec2 diff = p - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

float perlin(vec2 p) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(p, floor(p) + vec2(dx, dy));
        }
    }
    return surfletSum;
}

float random1(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453123);
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

float fbm2D(vec2 p) {
    // Initial values
    float value = 0.0;
    float persistence = 0.5;
    float amp = .5;
    float growth_factor = 3.0;
    int octaves = 8;
    //
    // Loop of octaves
    for (int i = 0; i < octaves; i++) {
        value += amp * interpNoise2D(p);

        p *= growth_factor;
        amp *= persistence;
    }
    return value;
}

float random1 (float n) {
    return fract(sin(n * 127.1) * 43758.5453123);
}

float interpNoise1D (float x) {
    int intX = int(floor(x));
    float fractX = fract(x);

    float v1 = random1(intX);
    float v2 = random1(intX + 1);
    return mix(v1, v2, fractX);
}

float fbm1D(float n) {
    float value = 0;
    float persistence = 0.5f;
    float amp = 0.5f;
    float growth_factor = 2.0;
    int octaves = 8;
    //
    for(int i = 1; i <= octaves; i++) {
        value += interpNoise1D(n) * amp;

        n *= growth_factor;
        amp *= persistence;
    }
    return value;

}


void main()
{    
//    float perlinNoise = perlin(vec2(fbm1D(13.f * fs_UV.y + sin(u_Time)), fbm1D(15.f * fs_UV.x + cos(u_Time))));
    float perlinInturbedFbmNoise = fbm2D(vec2(3.f * perlin(fs_UV) + sin(u_Time), 5.f * perlin(vec2(fs_UV.y, fs_UV.x)) + cos(u_Time)));
    vec2 uv = perlinInturbedFbmNoise/45.f + fs_UV;
    out_Col = texture(u_RenderedTexture, uv) + vec4(0, 0, 0.4, 0);
    normalize(out_Col);
}
