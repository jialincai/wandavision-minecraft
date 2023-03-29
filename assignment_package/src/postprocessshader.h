#pragma once

#include "shaderprogram.h"

class PostProcessShader : public ShaderProgram
{
public:

    int attrPos; // A handle for the "in" vec4 representing vertex position in the vertex shader
    int attrUV; // A handle for the "in" vec2 representing the UV coordinates in the vertex shader    

public:
    PostProcessShader(OpenGLContext* context);
    virtual ~PostProcessShader();

    // Sets up shader-specific handles
    virtual void setupMemberVars() override;
    // Draw the given object to our screen using this ShaderProgram's shaders
    void draw(Drawable &d, int textureSlot) override;
    void draw(Drawable &d, int textureSlot, int hextureSlot);
};
