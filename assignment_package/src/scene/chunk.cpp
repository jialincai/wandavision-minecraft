#include "chunk.h"
#include <unordered_set>
#include "sceneutils.h"
#include <iostream>


Chunk::Chunk(OpenGLContext* context, glm::vec2 pos) : Drawable(context), m_blocks(), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
                                                      pos(pos)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);

    // create transparent chunk child
    //TransparentChunk transparentVBOs = TransparentChunk(context, pos);
    transparent = new TransparentChunk(context, pos);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

BlockType Chunk::getAdjacentBlockAt(Direction direction, int x, int y, int z) {
    bool goToNeighbour = false;
    glm::ivec3 dir = getDirVecForDirection(direction);
    if( (direction == XPOS && x == 15) || (direction == XNEG && x == 0) ) {
        goToNeighbour = true;
        x = 15 - x;
    }
    else if( (direction == ZPOS && z == 15) || (direction == ZNEG && z == 0) ) {
        goToNeighbour = true;
        z = 15 - z;
    }
    if(goToNeighbour) {
        Chunk* neighbour = m_neighbors.at(direction);
        if(neighbour == nullptr) {
            return EMPTY; //if it was required to go to a neighbour, but a neighbour didn't exist, then we can assume an empty block
        }
        return neighbour->getBlockAt(x, y, z); //otherwise return the block from the existing appropriate neighbour
    }
    else {
        if (dir.y + y < 0) {
            return EMPTY;
        }
        return getBlockAt(dir.x + x, dir.y + y, dir.z + z); //if neighbour is not required, block exists within the bounds of the current chunk itself
    }
}

glm::vec2 Chunk::getChunkPos() const {
    return pos;
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::createVBOdata() {
    // DEPRECATED
    // use the section in Terrain::checkForWork(uint i) instead
    std::vector<glm::vec4> interleavedData;
    std::vector<GLuint> idx;
    m_count = idx.size();

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generateInterleavedVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufInterleavedVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(glm::vec4), interleavedData.data(), GL_STATIC_DRAW);
}

void Chunk::create(std::vector<glm::vec4> vboData, std::vector<GLuint> idxData)
{
    m_count = idxData.size();

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxData.size() * sizeof(GLuint), idxData.data(), GL_STATIC_DRAW);

    generateInterleavedVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufInterleavedVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(glm::vec4), vboData.data(), GL_STATIC_DRAW);

    this->m_hasVBOData = true;
}

void Chunk::createDouble(std::vector<glm::vec4> vboData, std::vector<GLuint> idxData,
                         std::vector<glm::vec4> vboDataT, std::vector<GLuint> idxDataT)
{
    m_count = idxData.size();

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxData.size() * sizeof(GLuint), idxData.data(), GL_STATIC_DRAW);

    generateInterleavedVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufInterleavedVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(glm::vec4), vboData.data(), GL_STATIC_DRAW);

    transparent->create(vboDataT, idxDataT);

    this->m_hasVBOData = true;
}

TransparentChunk::TransparentChunk(OpenGLContext* context, glm::vec2 pos) :
    Drawable(context), pos(pos)
{}

glm::vec2 TransparentChunk::getChunkPos() const {
    return pos;
}

void TransparentChunk::createVBOdata() {
    // also DEPRECATED
    // TODO: remove this the need to define a 'createVBOdata()' from Drawable
    m_count = idxTransparent.size();

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxTransparent.size() * sizeof(GLuint), idxTransparent.data(), GL_STATIC_DRAW);

    generateInterleavedVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufInterleavedVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, interleavedDataTransparent.size() * sizeof(glm::vec4), interleavedDataTransparent.data(), GL_STATIC_DRAW);
}

void TransparentChunk::create(std::vector<glm::vec4> vboDataT, std::vector<GLuint> idxDataT) {
    m_count = idxDataT.size();

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxDataT.size() * sizeof(GLuint), idxDataT.data(), GL_STATIC_DRAW);

    generateInterleavedVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufInterleavedVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboDataT.size() * sizeof(glm::vec4), vboDataT.data(), GL_STATIC_DRAW);
}
