#include "chunkvbodata.h"

ChunkVBOData::ChunkVBOData(Chunk* c) :
    mp_chunk(c),
    m_vboDataOpaque{}, m_vboDataTransparent{},
    m_idxDataOpaque{}, m_idxDataTransparent{}
{}
