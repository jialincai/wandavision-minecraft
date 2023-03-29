#version 150

in float dist;

out vec4 out_Col;

void main()
{
    float blackOrWhite = step(0, dist);
    out_Col = vec4(vec3(blackOrWhite), 1.0);
}
