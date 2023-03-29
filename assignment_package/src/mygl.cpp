#include "mygl.h"
#include "scene/sceneutils.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_progHexBounds(this), m_progToon(this), m_progSurfaceGlitch(this),
      m_progHexWalls(this), m_progShadow(this), m_progPostnoOp(this), m_progWater(this), m_progLava(this),
      m_progGreyscale(this), m_progPostGlitch(this),
      currentSurfaceShader(nullptr),
      currentPostProcessShader(nullptr), m_terrain(this),
      m_player(glm::vec3(48.f, 129.f, 48.f), m_terrain),
      m_currentMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()),
      m_currentSecsPassed(0.f),
      m_geomQuad(this),
      m_hex(this, 1.5f),
      terrainFrameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
      hexMapFrameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
      overlayFrameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
      shadowMapBuffer(this, 2048, 2048, 1.f)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);        

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}

void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    //instantiate the screen-spanning quadrangle used to draw our scene with post-processing pipeline
    m_geomQuad.createVBOdata();        

    //create the frame buffer for post-process shaders (milestone 2)
    terrainFrameBuffer.create();
    hexMapFrameBuffer.create();
    overlayFrameBuffer.create();

    // create additional frame buffer for shadow mapping (pt 1)
    shadowMapBuffer.create();

    // create surface shaders
    m_progLambert.create(":/glsl/lambert.vert.glsl",  ":/glsl/lambert.frag.glsl");
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    m_progHexBounds.create(":/glsl/hexbounds.vert.glsl", ":/glsl/hexbounds.frag.glsl");
    m_progToon.create(":/glsl/hexbounds.vert.glsl", ":/glsl/toon.frag.glsl");
    m_progSurfaceGlitch.create(":/glsl/glitch.vert.glsl",  ":/glsl/lambert.frag.glsl");
    m_progHexWalls.create(":/glsl/flat.vert.glsl", ":/glsl/hexwalls.frag.glsl");
    m_progShadow.create(":/glsl/shadowMap.vert.glsl",  ":/glsl/shadowMap.frag.glsl");

    // create post processing shaders
    m_progPostnoOp.create(":/glsl/noOp.vert.glsl", ":/glsl/noOp.frag.glsl");
    m_progWater.create(":/glsl/noOp.vert.glsl", ":/glsl/water.frag.glsl");
    m_progLava.create(":/glsl/noOp.vert.glsl", ":/glsl/lava.frag.glsl");
    m_progGreyscale.create(":/glsl/noOp.vert.glsl", ":/glsl/greyscale.frag.glsl");
    m_progPostGlitch.create(":/glsl/noOp.vert.glsl", ":/glsl/glitch.frag.glsl");


    m_progLambert.setModelMatrix(glm::mat4());

    std::shared_ptr<Texture> minecraftTextures = std::make_shared<Texture>(this);
    minecraftTextures->create(":/textures/minecraft_textures_all.png");
    minecraftTextures->load(0);
    m_textures.push_back(minecraftTextures);

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    // m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    currentSurfaceShader = &m_progLambert;
    currentPostProcessShader = &m_progPostnoOp;

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    m_terrain.multithreadedWork(m_player.mcr_position, m_player.mcr_prevPos, 1.f);

    //Casting the first hex on game startup
    castHex();
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    m_progHexBounds.setViewProjMatrix(viewproj);
    m_progToon.setViewProjMatrix(viewproj);
    m_progSurfaceGlitch.setViewProjMatrix(viewproj);
    m_progHexWalls.setViewProjMatrix(viewproj);

    terrainFrameBuffer.resize(w, h, 1.);
    terrainFrameBuffer.destroy();
    terrainFrameBuffer.create();

    hexMapFrameBuffer.resize(w, h, 1.);
    hexMapFrameBuffer.destroy();
    hexMapFrameBuffer.create();

    overlayFrameBuffer.resize(w, h, 1.);
    overlayFrameBuffer.destroy();
    overlayFrameBuffer.create();

    m_progLambert.setDimensions(glm::ivec2(w, h));
    m_progLava.setDimensions(glm::ivec2(w, h));
    m_progWater.setDimensions(glm::ivec2(w, h));
    m_progGreyscale.setDimensions(glm::ivec2(w,h));
    m_progToon.setDimensions(glm::ivec2(w,h));
    m_progSurfaceGlitch.setDimensions(glm::ivec2(w,h));
    m_progPostGlitch.setDimensions(glm::ivec2(w,h));

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    float dT = (QDateTime::currentMSecsSinceEpoch() - m_currentMSecsSinceEpoch) / 1000.f;
    m_player.tick(dT, m_inputs);
    m_currentMSecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
    m_currentSecsPassed = m_currentSecsPassed + dT;

    // compute the MVP matrix from LIGHT_DIRECTION (currently hard-coded)
    float near_plane = 1.0f, far_plane = 300.f;
    glm::mat4 depthProjectionMatrix = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, near_plane, far_plane);

    glm::vec3 light_position = glm::normalize(glm::vec3(0.5, 1, 0.75)) * 100.f + vec3(m_player.mcr_position.x, 129, m_player.mcr_position.z);
    glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(light_position), vec3(m_player.mcr_position.x, 129, m_player.mcr_position.z), glm::vec3(0,1,0));
    glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix;
    m_depthMVP = depthMVP;
    m_progLambert.setDepthMVP(m_depthMVP);
    m_progShadow.setDepthMVP(m_depthMVP);

    m_terrain.multithreadedWork(m_player.mcr_position, m_player.mcr_prevPos, dT);

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progInstanced.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progHexBounds.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progToon.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progSurfaceGlitch.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progHexWalls.setViewProjMatrix(m_player.mcr_camera.getViewProj());

    renderHexMap();
    performShadowMapPass();
    renderTerrain();
    performPostprocessRenderPass();
    performTimelineRenderPass();

    // bind minecraft block textures to texSlot 0 & pass to GPU
    m_textures.at(0)->bind(0);
    m_progFlat.setTextureSampler2D(0);

    // draws the world axis
    glDisable(GL_DEPTH_TEST);
    m_progFlat.setModelMatrix(glm::mat4());
    m_progHexWalls.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progFlat.draw(m_worldAxes);
    glEnable(GL_DEPTH_TEST);
}

void MyGL::setSurfaceShader() {
    switch(m_hex.getCurrentTimeline()) {
        case(TIMELINE_80S) : {
            currentSurfaceShader = &m_progToon; break;
        }
        case(TIMELINE_DYSTOPIAN) : {
            currentSurfaceShader = &m_progSurfaceGlitch; break;
        }
        default : {
        currentSurfaceShader = &m_progLambert; break;
        }
    }
    currentSurfaceShader->setTime(m_currentSecsPassed);
}

void MyGL::setPostProcessShader(bool timelinePass) {
    if(timelinePass) {
        switch(m_hex.getCurrentTimeline()) {
            case(TIMELINE_60S) : {
                currentPostProcessShader = &m_progGreyscale; break;
            }
            case(TIMELINE_DYSTOPIAN) : {
            currentPostProcessShader = &m_progPostGlitch; break;
            }
            default : {
                currentPostProcessShader = &m_progPostnoOp; break;
            }
        }
    }
    else {
        BlockType playerInBlockType = m_player.playerInBlockType(m_terrain);
        switch(playerInBlockType) {
            case(WATER) : {
                currentPostProcessShader = &m_progWater; break;
            }
            case(LAVA) : {
                currentPostProcessShader = &m_progLava; break;
            }
            default : {
                currentPostProcessShader = &m_progPostnoOp; break;
            }
        }
    }
    currentPostProcessShader->setTime(m_currentSecsPassed);
}

void frameBufferSetup(FrameBuffer &frameBuffer, int w, int h, unsigned int textureSlot) {
    // Render to our framebuffer rather than the viewport
    frameBuffer.bindFrameBuffer();
    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0,0,w,h);
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    frameBuffer.bindToTextureSlot(textureSlot);
}

void MyGL::renderHexMap() {
    frameBufferSetup(hexMapFrameBuffer, this->width(), this->height(), HEXMAP_FRAME_BUFFER_TEXTURE_SLOT);
    drawTerrain(&m_progHexBounds);
}


void MyGL::renderTerrain() {
    setSurfaceShader();
    frameBufferSetup(terrainFrameBuffer, this->width(), this->height(), TERRAIN_FRAME_BUFFER_TEXTURE_SLOT);
    shadowMapBuffer.bindToDepthTexture(SHADOW_MAP_TEXTURE_SLOT);
    drawTerrain(currentSurfaceShader);
    drawHex();
}

void MyGL::drawTerrain(SurfaceShader* surfaceShader) {
    ivec2 currZone(64 * floor(m_player.mcr_position.x / 64.f),
                   64 * floor(m_player.mcr_position.z / 64.f));
    m_terrain.draw(currZone.x - 128, currZone.x + 192, currZone.y - 128, currZone.y + 192, surfaceShader);
}

void MyGL::castHex() {
    //update Hex attributes
    m_hex.resetHexAttributes(m_player.mcr_position.x, m_player.mcr_position.z, m_currentSecsPassed);
    m_hex.cycleTimeline();

    //set the corresponding updated shader attributes
    m_progHexBounds.setHexCenter(m_hex.getHexCenter());
    m_progToon.setHexCenter(m_hex.getHexCenter());
    m_progSurfaceGlitch.setHexCenter(m_hex.getHexCenter());
    currentSurfaceShader->setHexCenter(m_hex.getHexCenter());
}

void MyGL::drawHex() {
    if(m_hex.canHexStillGrow(m_currentSecsPassed)) {
        m_hex.destroyVBOdata();
        m_hex.demarcateHexBoundaries(m_currentSecsPassed);
        m_progHexBounds.setHexRadius(m_hex.getHexRadius());
        m_progToon.setHexRadius(m_hex.getHexRadius());
        m_progSurfaceGlitch.setHexRadius(m_hex.getHexRadius());
        currentSurfaceShader->setHexRadius(m_hex.getHexRadius());
        m_hex.createVBOdata();
    }
    m_progHexWalls.setTime(m_currentSecsPassed);
    m_progHexWalls.draw(m_hex);
}

void MyGL::performPostprocessRenderPass()
{
    setPostProcessShader(false);
    frameBufferSetup(overlayFrameBuffer, this->width(), this->height(), OVERLAY_FRAME_BUFFER_TEXTURE_SLOT);
    currentPostProcessShader->draw(m_geomQuad, TERRAIN_FRAME_BUFFER_TEXTURE_SLOT);
}

void MyGL::performTimelineRenderPass()
{
    setPostProcessShader(true);
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    currentPostProcessShader->draw(m_geomQuad, OVERLAY_FRAME_BUFFER_TEXTURE_SLOT, HEXMAP_FRAME_BUFFER_TEXTURE_SLOT);
}

void MyGL::performShadowMapPass()
{
    shadowMapBuffer.bindFrameBuffer();
    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0, 0, 2048, 2048);

    // Clear the screen
    glClear(GL_DEPTH_BUFFER_BIT);

    // render scene
    ivec2 currZone(64 * floor(m_player.mcr_position.x / 64.f),
                   64 * floor(m_player.mcr_position.z / 64.f));
    m_terrain.draw(currZone.x - 128, currZone.x + 192, currZone.y - 128, currZone.y + 192, &m_progShadow);

    // Bind our texture in the requisite texture slot
    shadowMapBuffer.bindToDepthTexture(SHADOW_MAP_TEXTURE_SLOT);

    // pass created shadowMap to the lambert shader for actual rendering frfr this time
    m_progLambert.setShadowMapDepthTexture(SHADOW_MAP_TEXTURE_SLOT);
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_Left) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Up) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_Down) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = true;
    } else if (e->key() == Qt::Key_F) {
        if (!m_inputs.fPressed)
            m_inputs.flightMode = !m_inputs.flightMode;
        //std::cout << "toggled flightMode " << (m_inputs.flightMode ? "ON" : "OFF") << std::endl;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = true;
    } else if(e->key() == Qt::Key_H) {
        m_hex.updateGrowSpeed(50.f);
        castHex();
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Right) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_Left) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_Up) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_Down) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_F) {
        m_inputs.fPressed = false;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    float dx = width() / 2.f - e->pos().x();
    float dy = height() / 2.f - e->pos().y();

    if (dx != 0)
        m_player.rotateOnUpGlobal(dx / width() * 30.f);

    if (dy != 0)
        m_player.rotateOnRightLocal(dy / height() * 30.f);

    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        // left click - remove block
        m_player.removeBlock(&m_terrain);
    } else if (e->button() == Qt::RightButton) {
        // right click - place block
        m_player.placeBlock(&m_terrain, GRASS);
    }
}
