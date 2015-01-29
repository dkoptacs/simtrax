#ifndef _HWRT_LOADMEMORY_H_
#define _HWRT_LOADMEMORY_H_

#include "FourByte.h"
#include "Camera.h"
#include "Triangle.h"
#include "OBJLoader.h"
#include "OBJListLoader.h"
#include "IWLoader.h"
#include "Grid.h"
#include "BVH.h"

#include <vector>

#define USE_BVH 1

// Describes all of the parameters we will use to load memory
struct LoadMemoryParams
{
    // Memory info, Trax setup
    FourByte* mem;
    int       mem_size;
    int       num_rotation_threads;
    int       num_TMs;

    // Image data
    simtrax::Camera* camera;
    int       image_width;
    int       image_height;
    int       tile_width;
    int       tile_height;
    int       num_samples;
    int       ray_depth;
    float     epsilon;

    // Scene data
    float       background_color[3];
    const char* model_file;
    float       light_pos[3];
    int         grid_dimensions;
    bool        duplicate_bvh;
    bool        triangles_store_edges;

    // Subtrees
    int  subtree_size;
    bool pack_split_axis;
    bool pack_stream_boundaries;

    // Data to be returned
    BVH* bvh;
    int  start_wq;
    int  start_framebuffer;
    int  start_scene;
    int  start_matls;
    int  start_camera;
    int  start_bg_color;
    int  start_light;
    int  start_permutation;
    int  end_memory;

    LoadMemoryParams() :
        // Memory info, Trax setup
        mem(NULL),
        mem_size(0),
        num_rotation_threads(1),
        num_TMs(1),

        // Image data
        camera(NULL),
        image_width(128),
        image_height(128),
        tile_width(16),
        tile_height(16),
        num_samples(1),
        ray_depth(1),
        epsilon(1.e-4f),

        // Scene data
        model_file(NULL),
        grid_dimensions(-1),
        duplicate_bvh(false),
        triangles_store_edges(false),

        // Subtrees
        subtree_size(0),
        pack_split_axis(false),
        pack_stream_boundaries(false),

        // Data to be returned
        bvh(NULL),
        start_wq(0),
        start_framebuffer(0),
        start_scene(0),
        start_matls(0),
        start_camera(0),
        start_bg_color(0),
        start_light(0),
        start_permutation(0),
        end_memory(0)
    {
        light_pos[0] = 7.97f;
        light_pos[1] = 1.4f;
        light_pos[2] = -1.74f;
    };
};


void LoadMemory(LoadMemoryParams &paramsInOut);

#endif // _HWRT_LOADMEMORY_H_
