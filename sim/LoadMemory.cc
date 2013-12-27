#include "WinCommonNixFcns.h"
#include "LoadMemory.h"
#include "BVH.h"
#include "Material.h"
#include <string.h>
#include <limits.h>
#include <cstdlib>
#include <math.h>

void LoadMemory(FourByte* mem, BVH* &bvh, int size, int image_width, int image_height,
		int grid_dimensions,
                Camera* camera, const char* model_file,
                int& start_wq, int& start_framebuffer, int& start_scene,
		int& start_matls,
                int& start_camera, int& start_bg_color, int& start_light,
                int& end_memory, float *light_pos, int &start_permutation,
                int tile_width, int tile_height, int ray_depth, int num_samples,
		int num_rotation_threads, int num_TMs, int subtree_size,
		float epsilon, bool duplicate_BVH, bool triangles_store_edges, bool pack_split_axis, bool pack_stream_boundaries) {
  int permutation[] = { 151,160,137,91,90,15,
		    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
		    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
		    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
		    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
		    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
		    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
		    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
		    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
		    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
		    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
		    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
		    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

#include "Hammersley.h"

//   int tile_width = 16;
//   int tile_height = 16;

  // Setup memory for Assignment 1 ;)
  mem[0].uvalue = 0;
  // width, inv_width, fwidth
  mem[1].uvalue = image_width;
  mem[2].fvalue = 1.f/static_cast<float>(image_width);
  mem[3].fvalue = static_cast<float>(image_width);
  // height, inv_height, fheight
  mem[4].uvalue = image_height;
  mem[5].fvalue = 1.f/static_cast<float>(image_height);
  mem[6].fvalue = static_cast<float>(image_height);

  
  // Pointers (actually written at the end
//   mem[7].ivalue = start_fb
//   mem[8].ivalue = start_scene;
//   mem[9].uvalue = start_matls;
//   mem[10].uvalue = start_camera;
//   mem[11].uvalue = start_bg_color;
//   mem[12].uvalue = start_light;
//   mem[13].ivalue = start_permutation;
//   mem[14].uvalue = end_memory;
//   mem[15].uvalue = size-1;
  mem[16].ivalue = ray_depth;
  mem[17].ivalue = num_samples;
  mem[18].fvalue = epsilon;
//   mem[19].ivalue = start_hammersley;
  
// mem[20] was used for atomic_inc address, but does not use memory anymore, uses GlobalRegisterFile
// keep this memory position free for backwards compatability
  
// mem[21].ivalue = num_nodes;
// mem[22].ivalue = start_costs; 
  mem[23].ivalue = (int)log2(static_cast<float>(num_rotation_threads)); // + 1 for finer granularity on work assignments
// mem[24].ivalue = start_secondary_bvh;
  mem[25].ivalue = num_rotation_threads;
  //mem[26].ivalue = start_subtree_sizes;
  //mem[27].ivalue = start_rotated_flags;
  //mem[28].ivalue = start_triangles;
  //mem[29].ivalue = num_triangles;
  //mem[30].ivalue = start_tex_coords;
  //mem[31].ivalue = start_zb;
  //mem[32].ivalue = start_parent_pointers;
  //mem[33].ivalue = start_subtree_ids;
  //mem[34].ivalue = num_subtrees;
  mem[35].ivalue = num_TMs;
  //mem[36].ivalue = start_vertex_normals;
  //mem[37].ivalue = num_interior_subtrees;
    
  // Build the work queue
  start_wq = 39;
  printf("Work queue starts at %d (0x%08x)\n", start_wq, start_wq);
//   int start_queue_storage = start_wq + 2; // need cur_tile and num_tiles
//   int num_tiles = 0;
//   for (int y = 0; y < image_height; y += tile_height) {
//     for (int x = 0; x < image_width; x += tile_width) {
//       int start_x = x;
//       int stop_x  = (x+tile_width < image_width) ? x+tile_width : image_width;
//       int start_y = y;
//       int stop_y  = (y+tile_height < image_height) ? y+tile_height : image_height;
//       mem[start_queue_storage + num_tiles * 4 + 0].ivalue = start_x;
//       mem[start_queue_storage + num_tiles * 4 + 1].ivalue = stop_x;
//       mem[start_queue_storage + num_tiles * 4 + 2].ivalue = start_y;
//       mem[start_queue_storage + num_tiles * 4 + 3].ivalue = stop_y;
//       num_tiles++;
//     }
//   }
//   printf("Work queue ends at %d (0x%08x)\n", start_wq + num_tiles * 4 + 2,
//          start_wq + num_tiles * 4 + 2);
//   mem[start_wq + 0].ivalue = num_tiles;
//   printf("Have %d tiles\n", num_tiles);

  // num_pixels
  mem[start_wq + 0].ivalue = image_width * image_height;
  // cur_tile
  mem[start_wq + 1].ivalue = 0;
  start_framebuffer = start_wq + 2;
  mem[7].ivalue = start_framebuffer; // offset for frame buffer ?
  int num_pixels = image_width * image_height;
  const int channels = 3;
  int frame_size = (num_pixels * channels);
  int start_zbuffer = start_framebuffer + frame_size;
  int zsize = num_pixels;
  mem[31].ivalue = start_zbuffer;
  for (int i = 0; i < zsize; ++i) {
    mem[start_zbuffer + i].fvalue = 1000000;
  }
  start_scene = start_framebuffer + frame_size + zsize;
  start_permutation = start_scene;
  printf("FB starts at %d (0x%08x)\n", start_framebuffer, start_framebuffer);
  //printf("FB ends at %d (0x%08x)\n", start_scene - 1, start_scene - 1);


  // set triangles saving edges rather than points before doing all the loads
  Triangle::tri_stores_edges = triangles_store_edges;
  
  std::vector<Triangle*> *triangles = new std::vector<Triangle*>();
  std::vector<Material*> *matls = new std::vector<Material*>();
  std::vector<PPM*> textures;

  // if no model specified, don't try to load the scene
  if(model_file != NULL)
    {
      if (strstr(model_file, ".objl")) {
	OBJListLoader::LoadModel(model_file, triangles, matls, mem, INT_MAX, start_scene);
      } else if (strstr(model_file, ".obj")) {
	OBJLoader::LoadModel(model_file, triangles, matls, mem, INT_MAX, start_scene);
      } else if (strstr(model_file, ".iw")) {
	IWLoader::LoadModel(model_file, triangles, matls);
      } else {
	printf("ERROR: model file isn't .obj, .objl, or .iw\n");
	exit(-1);
      }
    
      mem[29].ivalue = (int)triangles->size();

      // write materials to memory
      start_matls = start_scene;
      // Add a grey material for objects without mat_id
      Material* grey = new Material(Vector3(0.78, 0.78, 0.78),
				    Vector3(0.78, 0.78, 0.78),
				    Vector3(0.78, 0.78, 0.78));
      //grey.LoadIntoMemory(start_scene, INT_MAX, mem);
      matls->push_back(grey);
      printf("Materials start at %d (0x%08x)\n", start_matls, start_matls);
      for (size_t i = 0; i < matls->size(); i++) {
	matls->at(i)->LoadIntoMemory(start_scene, INT_MAX, mem);
      }
      for (size_t i = 0; i < matls->size(); i++) {
	matls->at(i)->LoadTextureIntoMemory(start_scene, INT_MAX, mem);
      }
      printf("Materials end at %d (0x%08x)\n", start_scene, start_scene);
    

      //might not need this
      start_scene++;
      
      Grid* grid = NULL;
      if(grid_dimensions==-1)
	bvh = new BVH( triangles, subtree_size, duplicate_BVH, triangles_store_edges, pack_split_axis, pack_stream_boundaries);
      else
	grid = new Grid(triangles, triangles_store_edges, grid_dimensions);
      mem[21].ivalue = bvh->num_nodes;
      int scene_data = start_scene;
      printf("Scene starts at %d (0x%08x)\n", scene_data, scene_data);
      if(grid_dimensions==-1)
	bvh->LoadIntoMemory(scene_data, INT_MAX, mem);
      else
	grid->LoadIntoMemory(scene_data, INT_MAX, mem);
      start_scene = bvh->start_nodes;
      mem[8].ivalue = bvh->start_nodes;
      mem[22].ivalue = bvh->start_costs;
      mem[24].ivalue = bvh->start_secondary_nodes;  
      mem[26].ivalue = bvh->start_subtree_sizes;
      mem[27].ivalue = bvh->start_rotated_flags;
      mem[28].ivalue = bvh->start_tris;
      mem[30].ivalue = bvh->start_tex_coords;
      mem[32].ivalue = bvh->start_parent_pointers;
      mem[33].ivalue = bvh->start_subtree_ids;
      mem[34].ivalue = bvh->num_subtrees;
      mem[36].ivalue = bvh->start_vertex_normals;
      mem[37].ivalue = bvh->num_interior_subtrees;
      

      printf("Triangles start at %d (0x%08x)\n",bvh->start_tris, bvh->start_tris);
      
      // free up some memory we don't need during the simulation (they've already been loaded)
      for(size_t i=0; i < triangles->size(); i++)
	delete triangles->at(i);
      triangles->clear();
      // do not delete the triangles vector, BVH still uses it
      
      printf("Scene ends at %d (0x%08x)\n", scene_data, scene_data);
      start_camera = scene_data + 1;
      
      int begin_camera = start_camera;
      if(camera != NULL)
	{
	  printf("Starting camera at %d (0x%08x)\n", begin_camera, begin_camera);
	  camera->LoadIntoMemory(begin_camera, INT_MAX, mem);
	  printf("Camera ended at %d (0x%08x)\n", begin_camera, begin_camera);
	}
      start_bg_color = begin_camera + 1;
      printf("Background Color 0x%08x to 0x%08x\n", start_bg_color, start_bg_color+2);
      float bg_color[3] = {1.0f, 1.0f, 1.0f};
      for (int i = 0; i < 3; i++) {
	mem[start_bg_color + i].fvalue = bg_color[i];
      }
      
      start_light = start_bg_color + 3;
      printf("Light at 0x%08x to 0x%08x\n", start_light, start_light+2);
      for (int i = 0; i < 3; i++) {
	mem[start_light + i].fvalue = light_pos[i];
      }
      start_permutation = start_light + 3;
    } // end if(model_file != NULL)
    
    


  // add permutation table
  
  printf("Permutation table from 0x%08x to 0x%08x\n", start_permutation,
	 start_permutation + 511);
  for (int i = 0; i < 256; i++) {
    mem[start_permutation + i].ivalue = permutation[i];
    mem[start_permutation + i + 256].ivalue = permutation[i];
  }
  int start_hammersley = start_permutation + 512;
  printf("Hammersley table from 0x%08x to 0x%08x\n", start_hammersley,
	 start_hammersley + 511);
  for (int i = 0; i < 512; i++) {
    mem[start_hammersley + i].fvalue = hammersley[i];
  }
  end_memory = start_hammersley + 512;

  printf("\nMemory used: %d (0x%08x)\n", end_memory, end_memory);
  int image_size = image_height * image_width * 3;
  printf("Image size: %d\n", image_size);
  //  printf("Tile memory size: %d\n", num_tiles * 4);
  //  printf("Memory for everything else: %d\n", end_memory - image_size - num_tiles * 4);

  // Pointers
  mem[9].uvalue = start_matls;
  mem[10].uvalue = start_camera;
  mem[11].uvalue = start_bg_color;
  mem[12].uvalue = start_light;
  mem[13].ivalue = start_permutation;
  mem[14].uvalue = end_memory; // start free memory
  mem[15].uvalue = size-1; // end free memory
  mem[end_memory].uvalue = end_memory+2; // initial head pointer for the queue
  mem[end_memory+1].uvalue = end_memory+2; // initial tail pointer for the queue
  //mem[16].ivalue = ray_depth
  //mem[17].ivalue = num_samples
  //mem[18].fvalue = epsilon
  mem[19].ivalue = start_hammersley;
}
