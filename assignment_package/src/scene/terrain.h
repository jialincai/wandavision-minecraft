#pragma once
#include "scene/sceneutils.h"
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "chunk.h"

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

#include <algorithm>
#include <math.h>
#include <unistd.h>

#include <mutex>
#include <shared_mutex>
#include <thread>

#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "chunk.h"
#include "chunkvbodata.h"
#include "shaderprogram.h"
#include "cube.h"
#include "surfaceshader.h"
#include "perlinnoise.h"



using namespace std;
using namespace glm;

using mutex_type = std::shared_mutex;
using read_only_lock  = std::shared_lock<mutex_type>;
using updatable_lock = std::unique_lock<mutex_type>;

// Helper functions to convert (x, z) to and from hash map key
int64_t toKey(int x, int z);
glm::ivec2 toCoords(int64_t k);

// The container class for all of the Chunks in the game.
// Ultimately, while Terrain will always store all Chunks,
// not all Chunks will be drawn at any given time as the world
// expands.
class Terrain {
    static mutex_type m_sharedChunksLock;

private:
    // Stores every Chunk according to the location of its lower-left corner
    // in world space.
    // We combine the X and Z coordinates of the Chunk's corner into one 64-bit int
    // so that we can use them as a key for the map, as objects like std::pairs or
    // glm::ivec2s are not hashable by default, so they cannot be used as keys.
    std::unordered_map<int64_t, uPtr<Chunk>> m_chunks;


    // We will designate every 64 x 64 area of the world's x-z plane
    // as one "terrain generation zone". Every time the player moves
    // near a portion of the world that has not yet been generated
    // (i.e. its lower-left coordinates are not in this set), a new
    // 4 x 4 collection of Chunks is created to represent that area
    // of the world.
    // The world that exists when the base code is run consists of exactly
    // one 64 x 64 area with its lower-left corner at (0, 0).
    // When milestone 1 has been implemented, the Player can move around the
    // world to add more "terrain generation zone" IDs to this set.
    // While only the 3 x 3 collection of terrain generation zones
    // surrounding the Player should be rendered, the Chunks
    // in the Terrain will never be deleted until the program is terminated.
    std::unordered_set<int64_t> m_generatedTerrain;

    OpenGLContext* mp_context;

    /* After a BlockTypeWorker has filled a Chunk with the appropriate
       BlockType data, it will push a pointer to the Chunk onto this vector. */
    vector<Chunk*> m_chunksThatHaveBlockData;
    mutex m_chunksThatHaveBlockDataLock;

    /* After a VBOWorker has setup interleaved buffers, the data
       is pushed onto this vector. */
    vector<ChunkVBOData> m_chunksThatHaveVBOs;
    mutex m_chunksThatHaveVBOsLock;

    queue<vec2> m_zonesToInstantiate;
    mutex m_zonesToInstantiateLock;

    /* A float that keeps track of how many seconds have elapsed
       since the last time we've tested weather or not to expand the Terrain. */
    float m_tryExpansionTimer;

    /* A vector of threads that will check if there are
       Chunks that need BlockType data or buffer data populated. */
    vector<thread> m_spawnedThreads;
    array<queue<pair<uint, Chunk*>>, 15> m_threadQueues;   // Each threads to-do queue.
    array<mutex, 15> m_threadMutexes;                       // Each thread queue mutex.

    enum workType {BT, VBO, NONE, IZ};                          // Used to specify BlockType work or VBO work.

    /* The index of the next thread to delegate work to. */
    unsigned int m_threadIdx;

    /* The maximum number of threads the machine can handle concurrently. */
    uint m_maxThreads;

public:
    Terrain(OpenGLContext *context);
    ~Terrain();

    // Instantiates a new Chunk and stores it in
    // our chunk map at the given coordinates.
    // Returns a pointer to the created Chunk.
    Chunk* instantiateChunkAt(int x, int z);
    // Do these world-space coordinates lie within
    // a Chunk that exists?
    bool hasChunkAt(int x, int z) const;
    // Do these world-space coordinates lie within
    // an existing Terrain Zone?
    bool hasZoneAt(int x, int z) const;
    // Assuming a Chunk exists at these coords,
    // return a mutable reference to it
    uPtr<Chunk>& getChunkAt(int x, int z);
    // Assuming a Chunk exists at these coords,
    // return a const reference to it
    const uPtr<Chunk>& getChunkAt(int x, int z) const;
    // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    BlockType getBlockAt(int x, int y, int z) const;
    BlockType getBlockAt(glm::vec3 p) const;
    // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.
    void setBlockAt(int x, int y, int z, BlockType t);
    /* Instances a 4x4 chunk terrain zone and spawns BlockType workers. */
    void instantiateZoneAt(int x, int z);
    /* A helper function that destroys the VBO of all the chunks
       in the specifieid terrain zone. */
    void destroyZoneAt(int x, int z);
    /* For a 4x4 Chunk terrain zone, spawn VBO workers to setup
       VBO and index buffers. */
    void createZoneBuffers(int x, int z);

    //A functjion to add caves to terrain. Making a separate function mostly so its easier to comment
    //it out and prevent caves from rendering while testing to spead up the process.
    void renderCaves(int x, int z);
    void renderIceBiome(int x, int z, int maxHeight, int xFloor, int zFloor);
    void renderDesertBiome(int x, int z, int maxHeight);
    void renderMountainBiome(int x, int z, int maxHeight);
    void renderLakeBiome(int x, int z, int maxHeight, int xFloor, int zFloor);

    // Draws every Chunk that falls within the bounding box
    // described by the min and max coords, using the provided
    // ShaderProgram
    void draw(int minX, int maxX, int minZ, int maxZ, SurfaceShader *shaderProgram);

//--------------------------------------------------------------------------------
// Procedural Terrain Generation
//--------------------------------------------------------------------------------
    /* Given these coordinates, populate the corresponding chunk
       with the appropriate BlockTypes. */
    void generateChunkTerrain(int xIn, int zIn);
    /* Given these coords, return the height of the particular biome. */
    int procGrasslandHt(int x, int z) const;
    int procDesertHt(int x, int z) const;
    int procMountainHt(int x, int z) const;
    int procIslandHt(int x, int z) const;
    /* Given these coords, return the weight of interpolation between the
       temperature and humidity. */
    float interpolateHumidity(int x, int z) const;
    float interpolateTemperature(int x, int z) const;
    // Functions to draw a asset()
    void drawSnowTree(int x, int y, int z);
    void drawCactus(int x, int y, int z);
    void drawMushroom(int x, int y, int z);

//--------------------------------------------------------------------------------
// Multi-threading
//--------------------------------------------------------------------------------
    /* Check if the terrain needs to expand and spawn threads
       to concurrently handle setting BlockType and creating buffer data. */
    void multithreadedWork(vec3 posCurr, vec3 posPrev, float dT);
    /* Checks the 5x5 terrain zones surrounding the player. If any of these zones
       contain non-existing Chunks. Instantiate the Chunk, and pass them to BlockTypeWorker
       to be given BlockType data. If any Chunks fall out of renderable range. Destroy it's VBO data. */
    void tryExpansion(vec3 posCurr, vec3 posPrev);
    /* Checks m_completedChunks to see if any threads have completed its work. */
    void checkThreadResults();
    /* Every worker thread will be passed this function. Checks for
       work in the thread's queue and handles it. */
    void checkForWork(uint i);
};
