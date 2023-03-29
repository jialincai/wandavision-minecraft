#version 330
// ^ Change this to version 130 if you have compatibility issues
uniform float u_Time;
in vec4 fs_Pos;
in vec4 fs_Col;
out vec4 out_Col;

vec3 random3( vec2 p ) {
    return fract(sin(vec3(dot(p,vec2(127.1, 311.7)),
                          dot(p,vec2(269.5, 183.3)),
                          dot(p, vec2(420.6, 631.2))
                    )) * 43758.5453);
}

void main()
{
    out_Col = vec4(random3(10.f * fs_Pos.xz + sin(u_Time)), 0.8f) + vec4(0.5f, 0.05f, 0.05f, 0.f);
    clamp(out_Col, 0, 1);
}
