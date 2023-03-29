#include "terrain.h"
#include "sceneutils.cpp"
#include "proceduralterrainhelp.h"
#include <stdexcept>
#include <iostream>

mutex_type Terrain::m_sharedChunksLock;

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context),
      m_chunksThatHaveBlockData(), m_chunksThatHaveBlockDataLock(),
      m_chunksThatHaveVBOs(), m_chunksThatHaveVBOsLock(),
      m_zonesToInstantiate(), m_zonesToInstantiateLock(),
      m_tryExpansionTimer(0.f),
      m_spawnedThreads(), m_threadQueues(), m_threadMutexes(),
      m_threadIdx(0), m_maxThreads(thread::hardware_concurrency() - 1)
{
    /* Our implementation only supports a maximum of 15 threads. */
    if (m_maxThreads > 15) {
        m_maxThreads = 15;
    }

    for (uint i = 0; i < m_maxThreads; i++) {
        m_spawnedThreads.push_back(thread(&Terrain::checkForWork, this, i));
    }
}

Terrain::~Terrain() {
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}

bool Terrain::hasZoneAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 64.f));
    int zFloor = static_cast<int>(glm::floor(z / 64.f));
    read_only_lock lock(m_sharedChunksLock);
    return m_generatedTerrain.find(toKey(64 * xFloor, 64 * zFloor)) != m_generatedTerrain.end();
}

uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

void Terrain::instantiateZoneAt(int x, int z) {
    ivec2 zoneCoord = ivec2(64 * floor(x / 64.f), 64 * floor(z / 64.f));
    for(int i = zoneCoord.x; i < zoneCoord.x+64; i += 16) {
        for(int j = zoneCoord.y; j < zoneCoord.y+64; j += 16) {
            Chunk* c = instantiateChunkAt(i, j);
            m_threadMutexes[m_threadIdx].lock();
            m_threadQueues[m_threadIdx].push(pair<uint, Chunk*>(BT, c));
            m_threadMutexes[m_threadIdx].unlock();
            m_threadIdx = (m_threadIdx + 1) % m_maxThreads;
        }
    }
    m_generatedTerrain.insert(toKey(zoneCoord[0], zoneCoord[1]));
}

void Terrain::destroyZoneAt(int x, int z) {
    ivec2 zoneCoord = ivec2(64 * floor(x / 64.f), 64 * floor(z / 64.f));
    for(int i = zoneCoord.x; i < zoneCoord.x+64; i += 16) {
        for(int j = zoneCoord.y; j < zoneCoord.y+64; j += 16) {
            if (getChunkAt(i, j)->mcr_hasVBOData) {
                Chunk* c = getChunkAt(i, j).get();
                c->destroyVBOdata();
            }
        }
    }
}

void Terrain::createZoneBuffers(int x, int z) {
    ivec2 zoneCoord = ivec2(64 * floor(x / 64.f), 64 * floor(z / 64.f));
    for(int i = zoneCoord.x; i < zoneCoord.x+64; i += 16) {
        for(int j = zoneCoord.y; j < zoneCoord.y+64; j += 16) {
            Chunk* c = getChunkAt(i, j).get();
            c->creatingVBOData();
            m_threadMutexes[m_threadIdx].lock();
            m_threadQueues[m_threadIdx].push(pair<uint, Chunk*>(VBO, c));
            m_threadMutexes[m_threadIdx].unlock();
            m_threadIdx = (m_threadIdx + 1) % m_maxThreads;
        }
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    updatable_lock lock(m_sharedChunksLock);
    Chunk *cPtr;
    if (true) {
        uPtr<Chunk> chunk = mkU<Chunk>(mp_context, glm::vec2(x,z));
        cPtr = chunk.get();
        m_chunks[toKey(x, z)] = move(chunk);
    }

    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!

void Terrain::draw(int minX, int maxX, int minZ, int maxZ, SurfaceShader *shaderProgram) {
    glm::mat4 modelMatrix = glm::mat4(1.f);

    // draw opaque VBOs first
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if (chunk == nullptr || chunk->mcr_hasVBOData == false) {
                continue;
            }
             modelMatrix[3] = glm::vec4(x, 0, z, 1);
            shaderProgram->setModelMatrix(modelMatrix);
            shaderProgram->drawInterleaved(*chunk);
        }
    }

    // draw transparent VBOs on top
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if (chunk == nullptr || chunk->mcr_hasVBOData == false) {
                continue;
            }
             modelMatrix[3] = glm::vec4(x, 0, z, 1);
             shaderProgram->setModelMatrix(modelMatrix);
            shaderProgram->drawInterleaved(*(*chunk).transparent);
        }
    }
}

//--------------------------------------------------------------------------------
// Procedural Terrain Generation
//--------------------------------------------------------------------------------
//
// PRIMARY
void Terrain::generateChunkTerrain(int xIn, int zIn) {
    /* xFloor and zFloor represent the lower-left corner of the Chunk */
    int xFloor = static_cast<int>(glm::floor(xIn / 16.f)) * 16;
    int zFloor = static_cast<int>(glm::floor(zIn / 16.f)) * 16;

    /* SETTING BLOCK-TYPES */
    for (int x = xFloor; x < xFloor + 16; x++) {
        for (int z = zFloor; z < zFloor + 16; z++) {
            float temp = interpolateTemperature(x, z);
            float humidity = interpolateHumidity(x, z);
            float mountHt = procMountainHt(x, z);
            float grassHt = procGrasslandHt(x, z);
            float desertHt = procDesertHt(x, z);
            float islandHt = procIslandHt(x, z);

            float lowTempMix = mix(mountHt, grassHt, humidity);
            float highTempMix = mix(desertHt, islandHt, humidity);

            int maxHeight = floor(mix(lowTempMix, highTempMix, temp));

            // Cap the maximum height
            if (maxHeight > 254) {
                maxHeight = 254;
            }

            // Create steps
            float norMaxHeight = maxHeight / 254.f;
            norMaxHeight = round(norMaxHeight * 100) / 100;
            maxHeight = norMaxHeight * 254;

            // Set water or ice blocks
            for (int y = 128; y <= 138; y++) {
                read_only_lock lock(m_sharedChunksLock);
                if (temp >= 0.5) {
                    setBlockAt(x, y, z, WATER);
                } else {
                    if (y == 138)
                        setBlockAt(x, y, z, ICE);
                    else
                        setBlockAt(x, y, z, WATER);
                }
            }

            // Render Biomes
            if (temp < 0.5 && humidity >= 0.5) {
                renderIceBiome(x, z, maxHeight, xFloor, zFloor);
            } else if (temp >= 0.5 && humidity < 0.5) {
                renderDesertBiome(x, z, maxHeight);
            } else if (temp < 0.5 && humidity < 0.5) {
                renderMountainBiome(x, z, maxHeight);
            } else {
                renderLakeBiome(x, z, maxHeight, xFloor, zFloor);
            }

            m_sharedChunksLock.lock_shared();
            setBlockAt(x, 0, z, BEDROCK); //all y=0 should have unbreakable bedrock terrain
            m_sharedChunksLock.unlock_shared();

            //cave system
            renderCaves(x,z);
        }
    }
}

void Terrain::renderCaves(int x, int z) {
    for(int y = 1; y <= 130; y++) {
        read_only_lock lock(m_sharedChunksLock);
        float perlinNoise = perlin3d(0.05f * vec3(x, y, z));
        if(perlinNoise < 0) {
            if(y < 50) {
                setBlockAt(x, y, z, LAVA); //if perlin noise is negative and Y value is less than 25, set LAVA
            }
            else if(getBlockAt(x, y + 1, z) != WATER) {
                setBlockAt(x, y, z, EMPTY);
            }
        }
        else {
            setBlockAt(x, y, z, STONE);
        }
    }
}

void Terrain::renderIceBiome(int x, int z, int maxHeight, int xFloor, int zFloor) {
    read_only_lock lock(m_sharedChunksLock);
    for (int y = 128; y <= maxHeight; y++) {
        if (y == maxHeight) {
            setBlockAt(x, y, z, SNOW);
            if (random1(vec2(x, z)) < 0.02
                && maxHeight  > 138 && maxHeight < 160
                && x - 3 > xFloor && x + 3 < xFloor + 16
                && z - 3 > zFloor && z + 3 < zFloor + 16) {
                drawSnowTree(x, y, z);
            }
        } else {
            setBlockAt(x, y, z, DIRT);
        }
    }
}

void Terrain::renderMountainBiome(int x, int z, int maxHeight) {
    read_only_lock lock(m_sharedChunksLock);
    for (int y = 128; y <= maxHeight; y++) {
        if (y == maxHeight) {
            setBlockAt(x, y, z, SNOW);
        } else {
            setBlockAt(x, y, z, STONE);
        }
    }
}

void Terrain::renderDesertBiome(int x, int z, int maxHeight) {
    read_only_lock lock(m_sharedChunksLock);
    for (int y = 128; y <= maxHeight; y++) {
        if (random1(vec2(x, z)) < 0.00125
            && maxHeight  > 138 && maxHeight < 230) {
            drawCactus(x, y, z);
        }
        setBlockAt(x, y, z, DESERT);
    }
}

void Terrain::renderLakeBiome(int x, int z, int maxHeight, int xFloor, int zFloor) {
    read_only_lock lock(m_sharedChunksLock);
    for (int y = 128; y <= maxHeight; y++) {
        if (y == maxHeight) {
            if (random1(vec2(x, z)) < 0.01
                && maxHeight  < 138
                && x - 6 > xFloor && x + 6 < xFloor + 16
                && z - 6 > zFloor && z + 6 < zFloor + 16) {
                drawMushroom(x, y, z);
            }
            setBlockAt(x, y, z, GRASS);
        } else {
            setBlockAt(x, y, z, DIRT);
        }
    }
}


int Terrain::procGrasslandHt(int x, int z) const {
    float grid_size = 100;
    float min = 135;
    float max = 160;

    vec2 p = vec2(x, z) / grid_size;
    vec2 offset = vec2(fbm2D(p + vec2(9.5, 2.6), 3),
                       fbm2D(p + vec2(5.2, 1.3), 3));

    float perturbedWorley = worley(p + offset);
    float perturbedFbm = fbm2D(p + offset, 3);

    float height = perturbedFbm * (2.0 / 3.0) + perturbedWorley * (2.0 / 3.0);
    height = height * height;
    return remap(height, 0.0, 1.0, min, max);
}

int Terrain::procDesertHt(int x, int z) const {
    float grid_size = 100;
    float min = 131;
    float max = 180;

    vec2 p = vec2(x, z) / grid_size;
    PerlinNoise pn;
    vec2 perlinOut = vec2(pn.noise(p[0] + 3.5, p[1] + 1.7, 0),
                          pn.noise(p[0] + 9.3, p[1] + 0.0, 0));

    float height = fbm2D(abs(perlinOut), 3);
    return remap(height, 0.2, 0.6, min, max);
}

int Terrain::procMountainHt(int x, int z) const {
    float grid_size = 100;
    float min = 131;
    float max = 244;

    PerlinNoise pn;
    vec2 p = vec2(x, z);
    p = (p + random2(p)) / grid_size;

    float height = pn.noise(p[0], p[1], 0)
                 + 0.5  * pn.noise(2 * p[0], 2 * p[1], 0)
                 + 0.25 * pn.noise(4 * p[0], 4 * p[1], 0);
    height /= 1.75;
    height = pow(height, 3);
    return remap(height, 0.0, 0.30, min, max);
}

int Terrain::procIslandHt(int x, int z) const {
    float grid_size = 100;
    float min = 131;    // -25
    float max = 135;    // 35

    PerlinNoise pn;
    vec2 p = vec2(x, z);
    p = (p + random2(p)) / grid_size;

    float height = pn.noise(p[0], p[1], 0)
                 + 0.5  * pn.noise(2 * p[0], 2 * p[1], 0)
                 + 0.25 * pn.noise(4 * p[0], 4 * p[1], 0);
    height /= 1.75;
    height = pow(height, 3);
    return remap(height, 0.0, 0.30, min, max);
}

float Terrain::interpolateHumidity(int x, int z) const {
    float grid_size = 500;

    vec2 p = vec2(x, z);
    p = (p + random2(p)) / grid_size;

    PerlinNoise pn;
    float noise = pn.noise(p[0], p[1], 0);
    return smoothstep(0.45f, 0.55f, noise);
}

float Terrain::interpolateTemperature(int x, int z) const {
    float grid_size = 500;

    vec2 p = vec2(x, z) + 1746.5f;
    p = (p + random2(p)) / grid_size;

    PerlinNoise pn;
    float noise = pn.noise(p[0], p[1], 0);
    return smoothstep(0.45f, 0.55f, noise);
}

void Terrain::drawSnowTree(int x, int y, int z) {
    read_only_lock lock(m_sharedChunksLock);
    for (int i = 0; i < 10; i++) {
        int lg_radius = 3;
        int sm_radius = 2;
        int shrink = 6;     // higher = slower shrink the radius of foliage
        int start = remap(random1(vec2(x, y)), 0.f, 1.f, 1, 7);
        if (i > start && i % 2 == 0) {
            for (int j = -1 * (lg_radius - i/shrink); j <= lg_radius - i/shrink; j++) {
                 for (int k = -1 * (lg_radius - i/shrink); k <= lg_radius - i/shrink; k++) {
                     setBlockAt(x + j, y + i, z + k, LEAF);
                 }
            }
        }
        else if (i > start && i % 2 == 1) {
            for (int j = -1 * (sm_radius - i/shrink); j <= sm_radius - i/shrink; j++) {
                 for (int k = -1 * (sm_radius - i/shrink); k <= sm_radius - i/shrink; k++) {
                     setBlockAt(x + j, y + i, z + k, LEAF);
                 }
            }
        }

        if (i < 9) {
            setBlockAt(x, y + i, z, WOOD);
        }
    }
}

void Terrain::drawCactus(int x, int y, int z) {
    read_only_lock lock(m_sharedChunksLock);
    int cactus_height = remap(random1(vec2(x, y)), 0.f, 1.f, 3, 8);
    for (int i = 0; i < cactus_height; i++) {
        setBlockAt(x, y + i, z, CACTUS);
    }
}

void Terrain::drawMushroom(int x, int y, int z) {
    read_only_lock lock(m_sharedChunksLock);
    int mushroom_radius = remap(random1(vec2(x, y)), 0.f, 1.f, 3, 6);
    int cap_height = mushroom_radius * 2.5;
    int mushroom_height = remap(random1(vec2(x, y)), 0.f, 1.f, 15, 35);
    // Draw the Cap
    for (int i = 0; i < cap_height - 2; i++) {
        for (int j = -1 * mushroom_radius; j <= mushroom_radius; j++) {
             for (int k = -1 * mushroom_radius; k <= mushroom_radius; k++) {
                 if (abs(j) == mushroom_radius && abs(k) == mushroom_radius) {
                     setBlockAt(x + j, y + i, z + k, EMPTY);
                 } else {
                     setBlockAt(x + j, y + i + mushroom_height - cap_height, z + k, MUSHROOM_CAP);
                 }
             }
        }
    }
    for (int i = cap_height - 2; i < cap_height; i++) {
        for (int j = -1 * mushroom_radius + 1; j <= mushroom_radius - 1; j++) {
            for (int k = -1 * mushroom_radius + 1; k <= mushroom_radius - 1; k++) {
                setBlockAt(x + j, y + i + mushroom_height - cap_height, z + k, MUSHROOM_CAP);
            }
        }
    }

    // Draw the stem
    for (int i = 0; i <= mushroom_height - 2; i++) {
        setBlockAt(x, y + i, z, MUSHROOM_STEM);
    }
}

//--------------------------------------------------------------------------------
// Multi-threading
//--------------------------------------------------------------------------------
void Terrain::multithreadedWork(vec3 posCurr, vec3 posPrev, float dT) {
    m_tryExpansionTimer += dT;
    if (m_tryExpansionTimer < 0.5f) {
        return;
    }
    tryExpansion(posCurr, posPrev);
    checkThreadResults();
    m_tryExpansionTimer = 0.0f;
}

void Terrain::tryExpansion(vec3 posCurr, vec3 posPrev) {
    /* Remap player position to the terrain zone player in in. */
    ivec2 currZone(glm::floor(posCurr.x / 64.f) * 64, glm::floor(posCurr.z / 64.f) * 64);
    ivec2 prevZone(glm::floor(posPrev.x / 64.f) * 64, glm::floor(posPrev.z / 64.f) * 64);

    /* Figure out which Terrain Zones have fallen out of the
       renderable range, remove chunks from m_chunksWithVBOData. */
    for (int i = -128; i < 192; i+= 64) {
        for (int j = -128; j < 192; j+= 64) {
            if (prevZone.x + i < currZone.x - 128
             || prevZone.x + i > currZone.x + 192
             || prevZone.y + j < currZone.y - 128
             || prevZone.y + j > currZone.y + 192) {
                if (hasZoneAt(prevZone.x + i, prevZone.y + j) &&
                    getChunkAt(prevZone.x + i, prevZone.y + j)->mcr_hasVBOData) {
                    destroyZoneAt(prevZone.x + i, prevZone.y + j);
                }
            }
        }
    }

    /* Figure out which Terrain Zones need to be populated with BlockTypes
     * or sent to VBO workers. */
    for (int i = -128; i < 192; i += 64) {
        for (int j = -128; j < 192; j += 64) {
            /* Terrain zone does not yet exist. */
            if (!hasZoneAt(currZone[0] + i, currZone[1] + j)) {
                int zoneCoordX = (64 * floor((currZone[0] + i) / 64.f));
                int zoneCoordZ = (64 * floor((currZone[1] + j) / 64.f));
                instantiateZoneAt(zoneCoordX, zoneCoordZ);
            /* Terrain zone exists but has no VBO data. */
            } else if (!getChunkAt(currZone.x + i, currZone.y + j)->mcr_creatingVBOData) {
                createZoneBuffers(currZone.x + i, currZone.y + j);
            }
        }
    }
}

void Terrain::checkThreadResults() {
    /* Check if any thread has finished populating a Chunk
       with BlockType data. If so, clear the vector and
       send information along to VBOWorker. */
    m_chunksThatHaveBlockDataLock.lock();
    for (Chunk* c : m_chunksThatHaveBlockData) {
        m_threadMutexes[m_threadIdx].lock();
        m_threadQueues[m_threadIdx].push(pair(VBO, c));
        m_threadMutexes[m_threadIdx].unlock();
        m_threadIdx = (m_threadIdx + 1) % m_maxThreads;
    }
    m_chunksThatHaveBlockData.clear();
    m_chunksThatHaveBlockDataLock.unlock();

    /* Check if any thread has finished setting up VBO and index buffers.
       If so, send the data to the GPU and clear the vector. */
    m_chunksThatHaveVBOsLock.lock();
    for (ChunkVBOData &cd : m_chunksThatHaveVBOs) {
       cd.mp_chunk->createDouble(cd.m_vboDataOpaque, cd.m_idxDataOpaque, cd.m_vboDataTransparent, cd.m_idxDataTransparent);
    }
    m_chunksThatHaveVBOs.clear();
    m_chunksThatHaveVBOsLock.unlock();
}

void Terrain::checkForWork(uint thread_idx) {
    uint type;
    Chunk* ptr;

    auto &queue = m_threadQueues[thread_idx];
    auto &m = m_threadMutexes[thread_idx];

    while (true) {
        /* Reset */
        type = NONE;
        ptr = nullptr;

        /* Check for work */
        m.lock();
        if (!queue.empty()) {
            type = queue.front().first;
            ptr = queue.front().second;
            queue.pop();
        }
        m.unlock();

        /* Handle BlockType work */
        if (type == BT) {
            vec2 chunkPos = ptr->getChunkPos();
            generateChunkTerrain(chunkPos.x, chunkPos.y);
            m_chunksThatHaveBlockDataLock.lock();
            m_chunksThatHaveBlockData.push_back(ptr);
            m_chunksThatHaveBlockDataLock.unlock();
        } else if (type == VBO) {
            ChunkVBOData data(ptr);
            std::vector<glm::vec4> interleavedData;
            std::vector<GLuint> idx;
            uint id = 0;

            std::vector<glm::vec4> interleavedDataTransparent;
            std::vector<GLuint> idxTransparent;
            uint idTransparent = 0;

            for(int i = 0; i < 16; ++i) {
                for(int j = 0; j < 256; ++j) {
                    read_only_lock lock(m_sharedChunksLock);
                    for(int k = 0; k < 16; ++k) {
                        BlockType t = getBlockAt(ptr->getChunkPos()[0] + i, j, ptr->getChunkPos()[1] + k);
                        if(isBlockOpaque(t)) { // if the current block is not opaque, then check its 6 neighbours to decide if any of its face needs to be drawn
                            for(const auto &adjacentFace : adjacentBlockFaces) {
                                BlockType neighbouringBlock = ptr->getAdjacentBlockAt(adjacentFace.direction, i, j, k);
                                if(!isBlockOpaque(neighbouringBlock)) { //if a neighbouring block is empty, then draw that side of the face
                                    for(int b = 0; b <= 3; ++b) {
                                        interleavedData.push_back(adjacentFace.bufferData[b].pos + glm::vec4(i, j, k, 0));// + glm::vec4(ptr->getChunkPos().x, 0, ptr->getChunkPos().y, 0)); //position
                                        interleavedData.push_back(glm::vec4(adjacentFace.dirVec, 1)); //normal
                                        interleavedData.push_back(glm::vec4(getUV(t, adjacentFace.direction) + uvOffset(b), 1)); //color
                                    }
                                    idx.push_back(id);idx.push_back(id + 1);idx.push_back(id + 2);
                                    idx.push_back(id);idx.push_back(id + 2);idx.push_back(id + 3);
                                    id+= 4;
                                }
                            }
                        } else if (t != EMPTY) {
                            for(const auto &adjacentFace : adjacentBlockFaces) {
                                BlockType neighbouringBlock = ptr->getAdjacentBlockAt(adjacentFace.direction, i, j, k);
                                if(neighbouringBlock == EMPTY) { //if a neighbouring block is empty, then draw that side of the face
                                    for(int b = 0; b <= 3; ++b) {
                                        interleavedDataTransparent.push_back(adjacentFace.bufferData[b].pos + glm::vec4(i, j, k, 0));// + glm::vec4(ptr->getChunkPos().x, 0, ptr->getChunkPos().y, 0)); //position
                                        interleavedDataTransparent.push_back(glm::vec4(adjacentFace.dirVec, 1)); //normal
                                        interleavedDataTransparent.push_back(glm::vec4(getUV(t, adjacentFace.direction) + uvOffset(b), 1)); //color
                                    }
                                    idxTransparent.push_back(idTransparent);
                                    idxTransparent.push_back(idTransparent + 1);
                                    idxTransparent.push_back(idTransparent + 2);
                                    idxTransparent.push_back(idTransparent);
                                    idxTransparent.push_back(idTransparent + 2);
                                    idxTransparent.push_back(idTransparent + 3);
                                    idTransparent+= 4;
                                }
                            }
                        }
                    }
                }
            }
            ptr->transparent->interleavedDataTransparent = interleavedDataTransparent;
            ptr->transparent->idxTransparent = idxTransparent;

            data.m_vboDataOpaque = interleavedData;
            data.m_idxDataOpaque = idx;
            data.m_vboDataTransparent = interleavedDataTransparent;
            data.m_idxDataTransparent = idxTransparent;

            m_chunksThatHaveVBOsLock.lock();
            m_chunksThatHaveVBOs.push_back(data);
            m_chunksThatHaveVBOsLock.unlock();
        }
    }
}
