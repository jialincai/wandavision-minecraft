#pragma once
#include "framebuffer.h"
#include "postprocessshader.h"
#include "scene/hex.h"
#include "scene/quad.h"
#include "surfaceshader.h"
#ifndef MYGL_H
#define MYGL_H

#include "openglcontext.h"
#include "shaderprogram.h"
#include "scene/worldaxes.h"
#include "scene/camera.h"
#include "scene/terrain.h"
#include "scene/player.h"
#include "texture.h"

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <smartpointerhelp.h>
#include <qdatetime.h>

#include <thread>

class MyGL : public OpenGLContext
{
    Q_OBJECT
private:
    WorldAxes m_worldAxes; // A wireframe representation of the world axes. It is hard-coded to sit centered at (32, 128, 32).

    SurfaceShader m_progLambert;// A shader program that uses lambertian reflection
    SurfaceShader m_progFlat;// A shader program that uses "flat" reflection (no shadowing at all)
    SurfaceShader m_progInstanced;// A shader program that is designed to be compatible with instanced rendering
    SurfaceShader m_progHexBounds; // A shader program to calculate the black-and-white hex layout map to be sent to a separate frame buffer
    SurfaceShader m_progToon; // A shader program for the 80s timeline
    SurfaceShader m_progSurfaceGlitch; //A shader program for the dystopian timeline
    SurfaceShader m_progHexWalls; //A shader program to draw the hex walls
    SurfaceShader m_progShadow;// A shader program that is designed to calculate the parts of terrain in shadow

    PostProcessShader m_progPostnoOp; // A shader program for pass-through post processing
    PostProcessShader m_progWater; // A shader program to apply the post-processing underwater effect
    PostProcessShader m_progLava; // A shader program to apply the post-processing lava effect
    PostProcessShader m_progGreyscale; // A shader program for the 60s timeline    
    PostProcessShader m_progPostGlitch; // A shader program for the dystopian timeline

    SurfaceShader* currentSurfaceShader;
    PostProcessShader* currentPostProcessShader;

    std::vector<std::shared_ptr<Texture>> m_textures;
    Texture* mp_texCurrent;

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
                // Don't worry too much about this. Just know it is necessary in order to render geometry.

    Terrain m_terrain; // All of the Chunks that currently comprise the world.
    Player m_player; // The entity controlled by the user. Contains a camera to display what it sees as well.
    InputBundle m_inputs; // A collection of variables to be updated in keyPressEvent, mouseMoveEvent, mousePressEvent, etc.

    QTimer m_timer; // Timer linked to tick(). Fires approximately 60 times per second.
    qint64 m_currentMSecsSinceEpoch;
    float m_currentSecsPassed;

    glm::mat4 m_depthMVP; // depthMVP used for shadow mapping
    glm::mat4 m_depthBiasMVP; // depthBiasMVP used for shadow mapping

    Quad m_geomQuad;
    Hex m_hex;

    FrameBuffer terrainFrameBuffer;
    FrameBuffer hexMapFrameBuffer;
    FrameBuffer overlayFrameBuffer;

    ShadowMapFBO shadowMapBuffer;

    void moveMouseToCenter(); // Forces the mouse position to the screen's center. You should call this
                              // from within a mouse move event after reading the mouse movement so that
                              // your mouse stays within the screen bounds and is always read.

    void sendPlayerDataToGUI() const;

public:
    explicit MyGL(QWidget *parent = nullptr);
    ~MyGL();

    // Called once when MyGL is initialized.
    // Once this is called, all OpenGL function
    // invocations are valid (before this, they
    // will cause segfaults)
    void initializeGL() override;
    // Called whenever MyGL is resized.
    void resizeGL(int w, int h) override;
    // Called whenever MyGL::update() is called.
    // In the base code, update() is called from tick().
    void paintGL() override;

    // Called from paintGL().
    // Calls Terrain::draw().
    void renderTerrain();
    void drawTerrain(SurfaceShader* surfaceShader);

    //A function to render terrain as a black-and-white map and send it to a frame buffer.
    //This is to be used in post processing to apply the effect only to the terrain that
    //lies inside the hex.
    void renderHexMap();

    //Hex Functions

    //castHex() gets called on the keypress event which recasts the
    //hex considering the player at its center.
    void castHex();
    //drawHex() is used to update VBO data used to render the hex.
    void drawHex();

    void performTimelineRenderPass();
    void performPostprocessRenderPass();
    void setSurfaceShader();
    void setPostProcessShader(bool timelineRenderPass);
    void performShadowMapPass();

protected:
    // Automatically invoked when the user
    // presses a key on the keyboard
    void keyPressEvent(QKeyEvent *e);
    // Automatically invoked when the user
    // releases a key on the keyboard
    void keyReleaseEvent(QKeyEvent *e);
    // Automatically invoked when the user
    // moves the mouse
    void mouseMoveEvent(QMouseEvent *e);
    // Automatically invoked when the user
    // presses a mouse button
    void mousePressEvent(QMouseEvent *e);

private slots:
    void tick(); // Slot that gets called ~60 times per second by m_timer firing.

signals:
    void sig_sendPlayerPos(QString) const;
    void sig_sendPlayerVel(QString) const;
    void sig_sendPlayerAcc(QString) const;
    void sig_sendPlayerLook(QString) const;
    void sig_sendPlayerChunk(QString) const;
    void sig_sendPlayerTerrainZone(QString) const;
};


#endif // MYGL_H
