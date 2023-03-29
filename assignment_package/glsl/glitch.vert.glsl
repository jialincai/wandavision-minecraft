#version 150
// ^ Change this to version 130 if you have compatibility issues

uniform mat4 u_Model;
uniform mat4 u_ModelInvTr;
uniform mat4 u_ViewProj;
uniform float u_Time;
uniform vec2 u_HexCenter;
uniform float u_HexRadius;
uniform mat4 u_DepthMVP;      // MVP transformation matrix from light's POV

in vec4 vs_Pos;
in vec4 vs_Nor;
in vec4 vs_Col;

out vec4 fs_Nor;
out vec4 fs_LightVec;
out vec4 fs_Col;
const vec4 lightDir = normalize(vec4(0.5, 1, 0.75, 0));
out vec4 fs_LightDepth;

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
    float dist  = sdfHex(u_HexCenter - modelposition.xz, u_HexRadius * cos(radians(30.f)));
    float noise =  sin(u_Time * 35.f + modelposition.x) * fract(sin(modelposition.y)) / clamp(mod(modelposition.x, 3), 2, 10);
    int factor = int(clamp(mod(modelposition.z, 3), 0, 2));

    //add noise-based displacement if the vertex lies inside the hex
    if(dist < 0) modelposition[factor] += noise/10.f;

    fs_LightVec = (lightDir);  // Compute the direction in which the light source lies

    fs_LightDepth = u_DepthMVP * vec4(vec3(modelposition), 1);

    gl_Position = u_ViewProj * modelposition;// gl_Position is a built-in variable of OpenGL which is
                                             // used to render the final positions of the geometry's vertices

}
