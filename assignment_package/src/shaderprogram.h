#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <openglcontext.h>
#include <glm_includes.h>
#include <glm/glm.hpp>

#include "drawable.h"


class ShaderProgram
{
public:
    GLuint vertShader; // A handle for the vertex shader stored in this shader program
    GLuint fragShader; // A handle for the fragment shader stored in this shader program
    GLuint prog;       // A handle for the linked shader program stored in this class

    int unifSampler2D; // A handle to the "uniform" sampler2D that will be used to read the texture containing the scene render
    int unifHexSampler2D; // A handle to the "uniform" sampler2D that will be used to read the hex b/w map
    int unifTime; // A handle for the "uniform" float representing time in the shader
    int unifHexCenter; //A handle for the uniform vec2 representing the hex's center as (x,z)
    int unifHexRadius; //A hanlde for the uniform float representing the hex's radius
    int unifDimensions; // A handle to the "uniform" ivec2 that stores the width and height of the texture being rendered

public:
    ShaderProgram(OpenGLContext* context);
    // Sets up the requisite GL data and shaders from the given .glsl files
    void create(const char *vertfile, const char *fragfile);
    // Sets up shader-specific handles
    virtual void setupMemberVars() = 0;
    // Tells our OpenGL context to use this shader to draw things
    void useMe();

    // Pass the given texture to this shader on the GPU
    void setTextureSampler2D(int textureSlot);
    //Pass the hex b/w map to the shader on the GPU
    void setHextureSampler2D(int textureSlot);
    // Pass the time to the shader on the GPU
    void setTime(float t);
    //Pass the hex's center (x,z) to the GPU
    void setHexCenter(const glm::vec2 &center);
    //set screen dimensions
    void setDimensions(glm::ivec2 dims);
    //Pass the hex's radius to the GPU
    void setHexRadius(float r);
    // Draw the given object to our screen using this ShaderProgram's shaders
    void draw(Drawable &d);
    //Draw the given object to our screen using one single interleaved Vertex Buffer Object
    void drawInterleaved(Drawable &d);
    // Draw the given object to our screen multiple times using instanced rendering
    void drawInstanced(InstancedDrawable &d);
    //draw the screen-spanning quadrangle using texture stored in a frame buffer
    virtual void draw(Drawable &d, int textureSlot) = 0;
    // Utility function used in create()
    char* textFileRead(const char*);
    // Utility function that prints any shader compilation errors to the console
    void printShaderInfoLog(int shader);
    // Utility function that prints any shader linking errors to the console
    void printLinkInfoLog(int prog);

    QString qTextFileRead(const char*);

protected:
    OpenGLContext* context;   // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
                            // we need to pass our OpenGL context to the Drawable in order to call GL functions
                            // from within this class.
};


#endif // SHADERPROGRAM_H
