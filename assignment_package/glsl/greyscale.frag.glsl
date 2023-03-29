#version 150

in vec2 fs_UV;

out vec4 color;

uniform sampler2D u_RenderedTexture;
uniform sampler2D u_Hexture;

void main()
{
    vec4 orig_col = texture(u_RenderedTexture, fs_UV);
    vec4 hex_col = texture(u_Hexture, fs_UV);
    float grey = 0.21 * orig_col.r + 0.72 * orig_col.g + 0.07 * orig_col.b;
    vec4 grey_col = vec4(vec3(grey), orig_col.a);
    color = mix(grey_col, orig_col, hex_col.r);
}
