#version 330
// ^ Change this to version 130 if you have compatibility issues

uniform sampler2D u_Texture; // The texture to be read from by this shader
uniform float u_Time; // Time for animation !!

in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec4 fs_Col;
out vec4 out_Col;
in float dist;

void main()
{
    //for toon shading with a texture, we will map each pixel's colour to the
    //nearest corner of its square on the texture grid. This way we'll get a
    //flat-shaded square to give the artificial toon-like look.
    vec2 corner_UV = vec2(int(16 * fs_Col.r)/16.f, int(16 * fs_Col.g)/16.f);
    vec4 baseColor = texture(u_Texture, corner_UV);

    // animate if animation flag is 1.f
    if (fs_Col.b == 1.f) {
        float newUV_r = fs_Col.r + mod(u_Time * 0.02f, 1.f / 16.f);
        baseColor = texture(u_Texture, vec2(newUV_r, fs_Col.g));
    }

    //borrowed from lambertian shader
    float diffuseTerm = dot(normalize(fs_Nor), normalize(fs_LightVec));
    diffuseTerm = clamp(diffuseTerm, 0, 1);
    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;

    //lambertian shading color
    vec4 lambertian_base = texture(u_Texture, fs_Col.rg);
    vec4 lambertian_col = vec4(lambertian_base.rgb * lightIntensity, baseColor.a);

    //toon shading
    vec4 toonShade;
    if (lightIntensity > 0.95)      toonShade = vec4(1.0, 1.0, 1.0, 1.0);
    else if (lightIntensity > 0.75) toonShade = vec4(0.8, 0.8, 0.8, 1.0);
    else if (lightIntensity > 0.50) toonShade = vec4(0.6, 0.6, 0.6, 1.0);
    else if (lightIntensity > 0.25) toonShade = vec4(0.4, 0.4, 0.4, 1.0);
    else                            toonShade = vec4(0.2, 0.2, 0.2, 1.0);

    //toon-shaded color
    vec4 toon_col = baseColor * toonShade;

    //final color - deciding b/w lambertian VS toon based on the b/w hex map
    float insideOrOutsideHex = step(0, dist);
    out_Col = mix(toon_col, lambertian_col, insideOrOutsideHex);
}
