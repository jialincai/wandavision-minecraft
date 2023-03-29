#version 330
// Input vertex data, different for all executions of this shader.
// layout (location = 0) in vec3 vs_Pos;
in vec4 vs_Pos;

uniform mat4 u_Model;
// Values that stay constant for the whole mesh.
uniform mat4 u_DepthMVP;

void main()
{
    gl_Position = u_DepthMVP * vec4(vec3(u_Model * vs_Pos), 1.0);
}
