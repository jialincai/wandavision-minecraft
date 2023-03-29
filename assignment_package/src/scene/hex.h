#pragma once

#include "drawable.h"
#include "scene/sceneutils.h"

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Hex : public Drawable
{
private:
    float hexCentreX;
    float hexCentreZ;
    float maxLength;
    float timeSinceLastCast;
    float growSpeed;
    Timeline currentTimeline;

public:
    Hex(OpenGLContext* context, float speed);
    void resetHexAttributes(float centreX, float centreZ, float u_Time);
    void demarcateHexBoundaries(float u_Time);
    void updateGrowSpeed(float newSpeed);
    void cycleTimeline();
    bool canHexStillGrow(float u_Time);
    float currentFrameRadius(float u_Time);
    Timeline getCurrentTimeline() const;
    glm::vec2 getHexCenter() const;
    float getHexRadius() const;
    virtual void createVBOdata();
};
