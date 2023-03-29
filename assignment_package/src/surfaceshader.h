#pragma once

#include "shaderprogram.h"

class SurfaceShader : public ShaderProgram
{
public:

    int attrPos; // A handle for the "in" vec4 representing vertex position in the vertex shader
    int attrNor; // A handle for the "in" vec4 representing vertex normal in the vertex shader
    int attrCol; // A handle for the "in" vec4 representing vertex color in the vertex shader
    int attrPosOffset; // A handle for a vec3 used only in the instanced rendering shader
    int attrUV; // A handle for the "in" vec2 representing the UV coordinates in the vertex shader

    int unifModel; // A handle for the "uniform" mat4 representing model matrix in the vertex shader
    int unifModelInvTr; // A handle for the "uniform" mat4 representing inverse transpose of the model matrix in the vertex shader
    int unifViewProj; // A handle for the "uniform" mat4 representing the view matrix in the vertex shader
    int unifColor; // A handle for the "uniform" vec4 representing color of geometry in the vertex shader
    int unifCamera; // A handle for the "uniform" vec3 representing the camera's position in world space

    int unifDepthMVP; // A handle for the "uniform" mat4 representing the depth MVP used in shadow mapping
    int unifDepthBiasMVP; // A handle for the "uniform" mat4 representing the depth bias MVP used in shadow mapping
    int unifShadowMap; // A handle for the "uniform" sampler2D texture we get out from the shadowMapFBO

public:
    SurfaceShader(OpenGLContext* context);
    virtual ~SurfaceShader();

    // Sets up shader-specific handles
    virtual void setupMemberVars() override;
    // Draw the given object to our screen using this ShaderProgram's shaders
    void draw(Drawable &d);
    // Draw the given object to our screen using this ShaderProgram's shaders
    virtual void draw(Drawable &d, int textureSlot) override;
    //Draw the given object to our screen using one single interleaved Vertex Buffer Object
    void drawInterleaved(Drawable &d);
    // Draw the given object to our screen multiple times using instanced rendering
    void drawInstanced(InstancedDrawable &d);
    // Pass the given model matrix to this shader on the GPU
    void setModelMatrix(const glm::mat4 &model);
    // Pass the given Projection * View matrix to this shader on the GPU
    void setViewProjMatrix(const glm::mat4 &vp);
    // Pass the given color to this shader on the GPU
    void setGeometryColor(glm::vec4 color);
    // Pass the camera's world coordinates to the GPU
    void setCameraWorldPosition(const glm::vec3 &v);
    // Pass the depthMVP * depthBiasMVP matrix to the GPU
    void setDepthMVP(const glm::mat4 &mvp);
    void setDepthBiasMVP(const glm::mat4 &mvp);
    void setShadowMapDepthTexture(int textureSlot);
};
