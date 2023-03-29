#pragma once
#include "smartpointerhelp.h"
#include "../drawable.h"
#include <array>
#include <unordered_map>
#include <cstddef>

//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER,
    SNOW, LAVA, BEDROCK, DESERT, ICE,
    WOOD, LEAF, CACTUS, MUSHROOM_STEM, MUSHROOM_CAP
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

struct BufferData {
    glm::vec4 pos;
    BufferData(glm::vec4 position)
        :pos(position)
    {}
};

struct BlockFace {
    Direction direction;
    glm::vec3 dirVec;
    std::array<BufferData, 4> bufferData;
    BlockFace(Direction dir, glm::vec3 d, BufferData b1, BufferData b2, BufferData b3, BufferData b4)
        : direction(dir), dirVec(d), bufferData({b1, b2, b3, b4})
    {}
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};


// A transparentChunk is always a member variable of some
// chunk; it is a Drawable that only holds the transparent VBOs
// whereas the parent chunk only holds the opaque VBOs
class TransparentChunk : public Drawable {
private:
    /* The x and z coordinates of the lower left corner
     * of this Chunk*/
    glm::vec2 pos;
public:
    TransparentChunk(OpenGLContext* context, glm::vec2 pos);
    glm::vec2 getChunkPos() const;
    void createVBOdata() override;

    // transparent data
    std::vector<glm::vec4> interleavedDataTransparent;
    std::vector<GLuint> idxTransparent;
    void create(std::vector<glm::vec4> vboDataT, std::vector<GLuint> idxDataT);
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    /* The x and z coordinates of the lower left corner
     * of this Chunk*/
    glm::vec2 pos;

public:
    Chunk(OpenGLContext* context, glm::vec2 pos);
    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    BlockType getAdjacentBlockAt(Direction direction, int x, int y, int z);
    glm::vec2 getChunkPos() const;
    TransparentChunk* transparent;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);
    void createVBOdata() override;

    /* Given the interleaved VBO and index buffers. Send the data to the GPU. */
    void create(std::vector<glm::vec4> vboData, std::vector<GLuint> idxData);
    void createDouble(std::vector<glm::vec4> vboData, std::vector<GLuint> idxData,
                      std::vector<glm::vec4> vboDataT, std::vector<GLuint> idxDataT);
};
