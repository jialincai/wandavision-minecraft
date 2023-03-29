#pragma once
#include "chunk.h"
#include <unordered_set>

//frame buffers
const static unsigned int TERRAIN_FRAME_BUFFER_TEXTURE_SLOT = 2;
const static unsigned int HEXMAP_FRAME_BUFFER_TEXTURE_SLOT = 3;
const static unsigned int OVERLAY_FRAME_BUFFER_TEXTURE_SLOT = 4;
const static unsigned int SHADOW_MAP_TEXTURE_SLOT = 5;

//Hex-related constants
const static float MAX_HEX_RADIUS = 50.f;
const static int Y_PLANE_LEVEL = 128;

enum Timeline: unsigned int {
    TIMELINE_60S, TIMELINE_80S, TIMELINE_DYSTOPIAN
};

const static std::unordered_set<BlockType> emptyBlockTypes {
    EMPTY, WATER, LAVA, ICE, CACTUS
};

const static std::unordered_set<BlockType> translucentBlockTypes {
    WATER, LAVA, ICE, CACTUS
};

const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

const static std::array<BlockFace, 6> adjacentBlockFaces {
    BlockFace(XPOS, glm::vec3(1, 0, 0),
              BufferData(glm::vec4(1, 0, 1, 1)),
              BufferData(glm::vec4(1, 0, 0, 1)),
              BufferData(glm::vec4(1, 1, 0, 1)),
              BufferData(glm::vec4(1, 1, 1, 1))
              ),
    BlockFace(XNEG, glm::vec3(-1, 0, 0),
              BufferData(glm::vec4(0, 0, 0, 1)),
              BufferData(glm::vec4(0, 0, 1, 1)),
              BufferData(glm::vec4(0, 1, 1, 1)),
              BufferData(glm::vec4(0, 1, 0, 1))
              ),
    BlockFace(YPOS, glm::vec3(0, 1, 0),
              BufferData(glm::vec4(0, 1, 0, 1)),
              BufferData(glm::vec4(0, 1, 1, 1)),
              BufferData(glm::vec4(1, 1, 1, 1)),
              BufferData(glm::vec4(1, 1, 0, 1))
              ),
    BlockFace(YNEG, glm::vec3(0, -1, 0),
              BufferData(glm::vec4(0, 0, 0, 1)),
              BufferData(glm::vec4(1, 0, 0, 1)),
              BufferData(glm::vec4(1, 0, 1, 1)),
              BufferData(glm::vec4(0, 0, 1, 1))
              ),
    BlockFace(ZPOS, glm::vec3(0, 0, 1),
              BufferData(glm::vec4(0, 0, 1, 1)),
              BufferData(glm::vec4(1, 0, 1, 1)),
              BufferData(glm::vec4(1, 1, 1, 1)),
              BufferData(glm::vec4(0, 1, 1, 1))
              ),
    BlockFace(ZNEG, glm::vec3(0, 0, -1),
              BufferData(glm::vec4(1, 0, 0, 1)),
              BufferData(glm::vec4(0, 0, 0, 1)),
              BufferData(glm::vec4(0, 1, 0, 1)),
              BufferData(glm::vec4(1, 1, 0, 1))
              ),
};

static uint getXYZindex(Direction d) {
    if(d == XPOS || d == XNEG) {
        return 0;
    }
    if(d == YPOS || d == YNEG) {
        return 1;
    }
    if(d == ZPOS || d == ZNEG) {
        return 2;
    }
}

static bool isBlockTranslucent(BlockType t) {
    return translucentBlockTypes.find(t) != translucentBlockTypes.end();
}

static bool isBlockOpaque(BlockType type) {
    return emptyBlockTypes.find(type) == emptyBlockTypes.end();
}

static glm::ivec3 getDirVecForDirection(Direction direction) {
    glm::ivec3 dirVec;
    for(const auto &f: adjacentBlockFaces) {
        if(f.direction == direction) {
            dirVec = f.dirVec;
        }
    }
    return dirVec;
}

static glm::vec3 getUV(BlockType t, Direction direction) {
    switch (t) {
        case STONE:
            return glm::vec3(1.f / 16.f, 15.f / 16.f, -1);

        case DIRT:
            return glm::vec3(2.f / 16.f, 15.f / 16.f, -1);

        case GRASS:
            if (direction == YPOS)
                return glm::vec3(8.f / 16.f, 13.f / 16.f, -1);
            else if (direction == YNEG)
                return glm::vec3(2.f / 16.f, 15.f / 16.f, -1);
            else
                return glm::vec3(3.f / 16.f, 15.f / 16.f, -1);

        case WATER:
            return glm::vec3(14.f / 16.f, 2.f / 16.f, 1);

        case SNOW:
            return glm::vec3(2.f / 16.f, 11.f / 16.f, -1);

        case LAVA:
            return glm::vec3(13.f / 16.f, 1.f / 16.f, 1);

        case BEDROCK:
            return glm::vec3(1.f / 16.f, 14.f / 16.f, -1);

        case DESERT:
            return glm::vec3(0.f / 16.f, 3.f / 16.f, -1);

        case ICE:
            return glm::vec3(3.f / 16.f, 11.f / 16.f, -1);

        case WOOD:
            return glm::vec3(4.f / 16.f, 14.f / 16.f, -1);

        case LEAF:
            if (direction == YPOS)
                return glm::vec3(2.f / 16.f, 11.f / 16.f, -1);
            else
                return glm::vec3(5.f / 16.f, 12.f / 16.f, -1);

        case CACTUS:
            if (direction == YPOS || direction == YNEG)
                return glm::vec3(5.f / 16.f, 11.f / 16.f, -1);
            else
                return glm::vec3(6.f / 16.f, 11.f / 16.f, -1);

        case MUSHROOM_STEM:
            return glm::vec3(13.f / 16.f, 7.f / 16.f, -1);

        case MUSHROOM_CAP:
            if (direction == YNEG)
                return glm::vec3(14.f / 16.f, 7.f / 16.f, -1);
            else
                return glm::vec3(13.f / 16.f, 8.f / 16.f, -1);

        default:
            return glm::vec3();
    }
}

const static glm::vec3 uvOffset(int i) {
    switch (i) {
        case 1:
            return glm::vec3(1.f / 16.f, 0, 0);
        case 2:
            return glm::vec3(1.f / 16.f, 1.f / 16.f, 0);
        case 3:
            return glm::vec3(0, 1.f / 16.f, 0);
        default:
            return glm::vec3();
    }
};
