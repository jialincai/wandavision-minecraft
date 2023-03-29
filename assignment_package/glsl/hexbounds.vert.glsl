#version 150

in vec4 vs_Pos;
in vec4 vs_Col;
in vec4 vs_Nor;

uniform mat4 u_Model;
uniform mat4 u_ModelInvTr;
uniform  mat4 u_ViewProj;
uniform vec2 u_HexCenter;
uniform float u_HexRadius;

out vec4 fs_Col;
out vec4 fs_Nor;
out vec4 fs_LightVec;
out float dist;
const vec4 lightDir = normalize(vec4(0.5, 1, 0.75, 0));

float sdfHex(vec2 p, float s)
{
    const vec3 k = vec3(-0.866025404,0.5,0.577350269);
    p = abs(p);
    p -= 2.0*min(dot(k.xy,p),0.0)*k.xy;
    p -= vec2(clamp(p.x, -k.z*s, k.z*s), s);
    return length(p)*sign(p.y);
}

void main()
{
    fs_Col = vs_Col;
    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = vec4(invTranspose * vec3(vs_Nor), 0);
    vec4 modelposition = u_Model * vs_Pos;
    fs_LightVec = (lightDir);

    gl_Position = u_ViewProj * modelposition;
    dist  = sdfHex(u_HexCenter - modelposition.xz, u_HexRadius * cos(radians(30.f)));
}
