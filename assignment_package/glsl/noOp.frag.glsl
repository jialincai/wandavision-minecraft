#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec2 fs_UV;

uniform sampler2D u_RenderedTexture;

out vec4 out_Col;

void main()
{
    // Copy the color; there is no shading.
    out_Col = texture(u_RenderedTexture, fs_UV).rgba;
}
