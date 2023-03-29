#include "hex.h"
#include "sceneutils.h"

Hex::Hex(OpenGLContext *context, float speed) :
    Drawable(context), hexCentreX(), hexCentreZ(), maxLength(), timeSinceLastCast(), growSpeed(speed), currentTimeline(TIMELINE_DYSTOPIAN)
{}

void Hex::resetHexAttributes(float centreX, float centreZ, float u_Time) {
    this->hexCentreX = centreX;
    this->hexCentreZ = centreZ;
    this->timeSinceLastCast = u_Time;
    this->maxLength = 0.f;
}

void Hex::demarcateHexBoundaries(float u_Time) {
    maxLength = glm::min(currentFrameRadius(u_Time), MAX_HEX_RADIUS);
}

bool Hex::canHexStillGrow(float u_Time) {
    return currentFrameRadius(u_Time) < MAX_HEX_RADIUS;
}

float Hex::currentFrameRadius(float u_Time) {
    return growSpeed * (u_Time - timeSinceLastCast);
}

void Hex::updateGrowSpeed(float newSpeed) {
    this->growSpeed = newSpeed;
}

void Hex::cycleTimeline() {
    currentTimeline = Timeline( (uint(currentTimeline) + 1) % 3 );
}

Timeline Hex::getCurrentTimeline() const {
    return currentTimeline;
}

glm::vec2 Hex::getHexCenter() const {
    return glm::vec2(hexCentreX, hexCentreZ);
}

float Hex::getHexRadius() const {
    return maxLength;
}

void Hex::createVBOdata()
{
    std::vector<glm::vec4> pos;
    std::vector<GLuint> idx;
    glm::vec4 initPoint = {maxLength, Y_PLANE_LEVEL, 0.f , 1.f};
    for(int i = 0; i < 6; i++) {

        glm::mat4 rotateAboutY = glm::rotate(glm::mat4(1.f), glm::radians(60.f), glm::vec3(0,1,0));
        glm::mat4 translateHexPos = glm::translate(glm::mat4(1.f), glm::vec3(hexCentreX, Y_PLANE_LEVEL, hexCentreZ));
        glm::mat4 translateHexNeg = glm::translate(glm::mat4(1.f), glm::vec3(hexCentreX, -Y_PLANE_LEVEL, hexCentreZ));
        glm::vec4 nextPoint = rotateAboutY * initPoint;
        pos.push_back(translateHexNeg * initPoint);
        pos.push_back(translateHexNeg * nextPoint);
        pos.push_back(translateHexPos * nextPoint);
        pos.push_back(translateHexPos * initPoint);

        idx.push_back(4*i);
        idx.push_back(4*i + 1);
        idx.push_back(4*i + 2);

        idx.push_back(4*i);
        idx.push_back(4*i + 2);
        idx.push_back(4*i + 3);
        initPoint = nextPoint;
    }

    m_count = idx.size();

    // Create a VBO on our GPU and store its handle in bufIdx
    generateIdx();
    // Tell OpenGL that we want to perform subsequent operations on the VBO referred to by bufIdx
    // and that it will be treated as an element array buffer (since it will contain triangle indices)
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    // Pass the data stored in cyl_idx into the bound buffer, reading a number of bytes equal to
    // CYL_IDX_COUNT multiplied by the size of a GLuint. This data is sent to the GPU to be read by shader programs.
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    // The next few sets of function calls are basically the same as above, except bufPos and bufNor are
    // array buffers rather than element array buffers, as they store vertex attributes like position.
    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec4), pos.data(), GL_STATIC_DRAW);
}
