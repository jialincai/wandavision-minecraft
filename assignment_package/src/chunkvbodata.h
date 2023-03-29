#pragma once
#include "scene/chunk.h"
#include <vector>

using namespace std;
using namespace glm;

class ChunkVBOData
{
private:
    Chunk* mp_chunk;
    vector<glm::vec4> m_vboDataOpaque, m_vboDataTransparent;
    vector<GLuint> m_idxDataOpaque, m_idxDataTransparent;

    friend class Terrain;

public:
    ChunkVBOData(Chunk* c);
};
