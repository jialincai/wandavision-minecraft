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

void main()
{
    float perlinNoise = perlin(vec2(12.f * fs_UV.x + cos(u_Time * 0.2f), 10.f * fs_UV.y - sin(u_Time * 0.5f)));
    vec2 uv = perlinNoise/35.f + fs_UV;
    out_Col = texture(u_RenderedTexture, uv) + vec4(0.8, 0.2, 0.2, 0);
    normalize(out_Col);
}
