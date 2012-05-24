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

void LoadMemory(FourByte* mem, BVH* &bvh, int size, int image_width, int image_height,
		int grid_dimensions,
                Camera* camera, const char* model_file,
                int& start_wq, int& start_framebuffer, int& start_scene,
		int& start_matls,
                int& start_camera, int& start_bg_color, int& start_light,
                int& end_memory, float *light_pos, int& start_permutation,
                int tile_width, int tile_height, int ray_depth, int num_samples,
		int num_rotation_threads, int num_TMs, int subtree_size,
		float epsilon, bool duplicate_BVH = false, bool triangles_store_edges=false);


#endif // _HWRT_LOADMEMORY_H_
