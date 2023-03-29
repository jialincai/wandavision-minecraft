#version 150

in vec2 fs_UV;

out vec4 color;

uniform sampler2D u_RenderedTexture;
uniform sampler2D u_Hexture;
uniform float u_Time;

void main()
{
    float time = u_Time * 1.5f;
    time *= 2;

    vec4 orig_col = texture(u_RenderedTexture, fs_UV);
    vec4 hex_col = texture(u_Hexture, fs_UV);

    float amountx = fract(sin(time)) *step(0.9,smoothstep(0.5,0.9,sin(time))) * 0.01;
    float amounty = fract(sin(time + 1.f)) *step(0.9,smoothstep(0.5,0.9,sin(time + 1.f))) * 0.03;
    vec4 col;
    col.r = texture( u_RenderedTexture, vec2(fs_UV.x + amountx,fs_UV.y - amounty) ).r;
    col.g = texture( u_RenderedTexture, fs_UV ).g;
    col.b = texture( u_RenderedTexture, vec2(fs_UV.x - amountx,fs_UV.y + amounty) ).b;
    col *= (1.0 - amountx * 0.5);
    col.a = orig_col.a;

    color = mix(col, orig_col, hex_col.r);
}
