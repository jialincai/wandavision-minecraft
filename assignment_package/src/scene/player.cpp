#include "player.h"
#include "scene/sceneutils.h"
#include <QString>
#include <iostream>
using namespace glm;

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain, input);
}

void Player::processInputs(InputBundle &inputs) {
    float a = 10.f; // scale val

    if (inputs.flightMode) {
        if (inputs.wPressed)
            m_acceleration = a * m_forward;

        if (inputs.sPressed)
            m_acceleration = -a * m_forward;

        if (inputs.dPressed)
            m_acceleration = a * m_right;

        if (inputs.aPressed)
            m_acceleration = -a * m_right;

        if (inputs.ePressed)
            m_acceleration = a * m_up;

        if (inputs.qPressed)
            m_acceleration = -a * m_up;

        // if no inputs provided, reset v & a to default (0, 0, 0)
        if (!(inputs.wPressed || inputs.sPressed || inputs.dPressed ||
              inputs.aPressed || inputs.ePressed || inputs.qPressed)) {
            m_velocity = vec3();
            m_acceleration = vec3();
        }
    } else {
        if (inputs.wPressed)
            m_acceleration = a * normalize(vec3(m_forward.x, 0, m_forward.z));

        if (inputs.sPressed)
            m_acceleration = -a * normalize(vec3(m_forward.x, 0, m_forward.z));

        if (inputs.dPressed)
            m_acceleration = a * normalize(vec3(m_right.x, 0, m_right.z));

        if (inputs.aPressed)
            m_acceleration = -a * normalize(vec3(m_right.x, 0, m_right.z));

        if (!(inputs.wPressed || inputs.sPressed || inputs.dPressed ||
              inputs.aPressed )) {
            m_velocity = vec3();
            m_acceleration = vec3();
        }
    }
}

// copied from Minecraft Grid Marching lecture
bool gridMarch(vec3 rayOrigin, vec3 rayDirection, const Terrain &terrain, float *out_dist, ivec3 *out_blockHit) {
    float maxLen = length(rayDirection); // Farthest we search
    ivec3 currCell = ivec3(floor(rayOrigin));
    rayDirection = normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        ivec3 offset = ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

void Player::computePhysics(float dT, const Terrain &terrain, InputBundle &inputs) {
    // handle lag
    if (dT > 0.1f ) {
        dT = 0.1f;
    }

    // normalize to rescale later
    if (m_acceleration != vec3(0.f)) {
        if (!inputs.flightMode) {
            m_acceleration = normalize(vec3(m_acceleration.x, 0.f, m_acceleration.z));
        } else {
            m_acceleration = normalize(m_acceleration);
        }
    }

    vec3 direction = vec3();

    if (!inputs.flightMode) {
        // check if onGround
        bool onGround = false;
        bool isSwimming = false;
        for (int i = 0; i <= 1; ++i) {
            for (int j = 0; j <= 1; ++j) {
                float out_dist = 0.f;
                ivec3 out_blockHit = ivec3();

                BlockType blockHit = terrain.getBlockAt(m_position + glm::vec3(i - 0.5f, -1.f, j - 0.5f));
                if (isBlockOpaque(blockHit) || isBlockTranslucent(blockHit)) {
                    if(isBlockTranslucent(blockHit)) {
                        isSwimming = true;
                        break;
                    }
                    else {
                        onGround = true;
                        m_velocity.y = 0.f;
                    }
                    break;
                }
            }
        }

        // handle jumping
        if (onGround) {
            if (inputs.spacePressed) {
                m_acceleration.y = 20.f;
            } else {
                m_acceleration.y = 0.f;
            }
        } else {
            m_acceleration.y = -20.f;
        }

        m_acceleration *= 100.f * dT;
        m_velocity += m_acceleration * dT;
        m_velocity *= .95;
        if(isSwimming) {
            m_velocity *= 0.67f;
            if(inputs.spacePressed) {
                m_velocity.y = std::fabs(m_velocity.y); //allow swimming upwards when holding spacebar
            }
        }
        direction = m_velocity * dT;


        handleCollision(terrain, &direction);
    } else {
        // flight mode!
        m_acceleration *= 1500.f * dT;
        m_velocity += m_acceleration * dT;
        m_velocity *= .95;
        direction = m_velocity * dT;
    }

    float scale = 1.12f;
    //m_acceleration *= scale;
    //m_velocity *= scale;

    // move character with collision taken into account
    moveAlongVector(direction);
}

void Player::handleCollision(const Terrain &terrain, vec3 *direction) {
    float out_dist = 0.f;
    ivec3 out_blockHit = ivec3();

    std::vector<vec3> vertices;
    for (int y = 0; y <= 2; y++) {
        vertices.push_back(m_position + vec3(-0.49, y, -0.49));
        vertices.push_back(m_position + vec3(-0.49, y, +0.49));
        vertices.push_back(m_position + vec3(+0.49, y, -0.49));
        vertices.push_back(m_position + vec3(+0.49, y, +0.49));
    }

    std::array<Direction, 3> dirsToCheck = {XPOS, YPOS, ZPOS};
    for(const auto& dir : dirsToCheck) {
        uint xyzIndex = getXYZindex(dir);

        for (vec3& v : vertices) {
            vec3 playerMovementDirection = *direction * adjacentBlockFaces.at(dir).dirVec;
            // if moving in that direction hits a block, check if it is not translucent and adjust accordingly
            if (gridMarch(v, playerMovementDirection, terrain, &out_dist, &out_blockHit)) {
                BlockType blockHit = terrain.getBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z);
                if (blockHit != WATER && blockHit != LAVA) {
                    if (out_dist > 0.01f) {
                        // slow down if big dist from hitting block
                        (*direction)[xyzIndex] = sign((*direction)[xyzIndex]) * (glm::min(abs((*direction)[xyzIndex]), out_dist) - 0.0001f);
                    } else {
                        (*direction)[xyzIndex] = 0.f;
                        break;
                    }
                }
            }
        }
    }
}

BlockType Player::playerInBlockType(const Terrain &terrain) {
    return terrain.getBlockAt(mcr_camera.mcr_position.x, mcr_camera.mcr_position.y, mcr_camera.mcr_position.z);
}

void Player::removeBlock(Terrain *terrain) {
    vec3 dir = normalize(m_forward) * 3.f; // change dist to 3 units away max
    float out_dist = 0.f;
    ivec3 out_blockHit = ivec3();

    if (gridMarch(m_camera.mcr_position, dir, *terrain, &out_dist, &out_blockHit)) {
//        std::cout << "removed block" << std::endl;

        // remove block @ out_blockHit pos in terrain unless it is BEDROCK which is unremovable (milestone 2)
        if(terrain->getBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z) != BEDROCK) {
        terrain->setBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z, EMPTY);
        }
    }
}

void Player::placeBlock(Terrain *terrain, BlockType type) {
    vec3 dir = normalize(m_forward) * 3.f; // change dist to 3 units away max
    float out_dist = 0.f;
    ivec3 out_blockHit = ivec3();

    if (gridMarch(m_camera.mcr_position, dir, *terrain, &out_dist, &out_blockHit)) {
//        std::cout << "placed block" << std::endl;

        vec3 point = m_camera.mcr_position + normalize(dir) * out_dist;
        vec3 center = vec3(out_blockHit) + vec3(0.5, 0.5, 0.5);

        vec3 diff = abs(point - center);
        if (diff.x > diff.y && diff.x > diff.z)
            diff = sign(point - center) * vec3(1, 0, 0);
        else if (diff.y > diff.x && diff.y > diff.z)
            diff = sign(point - center) * vec3(0, 1, 0);
        else if (diff.z > diff.x && diff.z > diff.y)
            diff = sign(point - center) * vec3(0, 0, 1);

        // place block at offset-ed position
        vec3 target = vec3(out_blockHit) + diff;
        terrain->setBlockAt(target.x, target.y, target.z, type);
    }
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
