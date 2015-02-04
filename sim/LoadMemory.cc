#include "WinCommonNixFcns.h"
#include "LoadMemory.h"
#include "BVH.h"
#include "Material.h"
#include <string.h>
#include <limits.h>
#include <cstdlib>
#include <math.h>

using namespace simtrax;

void LoadMemory(LoadMemoryParams &pio)
{
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


  // Setup pio.memory
  pio.mem[0].uvalue = 0;
  // width, inv_width, fwidth
  pio.mem[1].uvalue = pio.image_width;
  pio.mem[2].fvalue = 1.f/static_cast<float>(pio.image_width);
  pio.mem[3].fvalue = static_cast<float>(pio.image_width);
  // height, inv_height, fheight
  pio.mem[4].uvalue = pio.image_height;
  pio.mem[5].fvalue = 1.f/static_cast<float>(pio.image_height);
  pio.mem[6].fvalue = static_cast<float>(pio.image_height);


  // Pointers (actually written at the end
//   pio.mem[7].ivalue = start_fb
//   pio.mem[8].ivalue = start_scene;
//   pio.mem[9].uvalue = start_matls;
//   pio.mem[10].uvalue = start_camera;
//   pio.mem[11].uvalue = start_bg_color;
//   pio.mem[12].uvalue = start_light;
//   pio.mem[13].ivalue = start_permutation;
//   pio.mem[14].uvalue = end_pio.memory;
//   pio.mem[15].uvalue = size-1;
  pio.mem[16].ivalue = pio.ray_depth;
  pio.mem[17].ivalue = pio.num_samples;
  pio.mem[18].fvalue = pio.epsilon;
//   pio.mem[19].ivalue = start_hammersley;
  
// pio.mem[20] was used for atomic_inc address, but does not use pio.memory anymore, uses GlobalRegisterFile
// keep this pio.memory position free for backwards compatability
  
// pio.mem[21].ivalue = num_nodes;
// pio.mem[22].ivalue = start_costs;
  pio.mem[23].ivalue = (int)log2f(static_cast<float>(pio.num_rotation_threads)); // + 1 for finer granularity on work assignments
// pio.mem[24].ivalue = start_secondary_bvh;
  pio.mem[25].ivalue = pio.num_rotation_threads;
  //pio.mem[26].ivalue = start_subtree_sizes;
  //pio.mem[27].ivalue = start_rotated_flags;
  //pio.mem[28].ivalue = start_triangles;
  //pio.mem[29].ivalue = num_triangles;
  //pio.mem[30].ivalue = start_tex_coords;
  //pio.mem[31].ivalue = start_zb;
  //pio.mem[32].ivalue = start_parent_pointers;
  //pio.mem[33].ivalue = start_subtree_ids;
  //pio.mem[34].ivalue = num_subtrees;
  pio.mem[35].ivalue = pio.num_TMs;
  //pio.mem[36].ivalue = start_vertex_normals;
  //pio.mem[37].ivalue = num_interior_subtrees;
  pio.mem[38].ivalue = pio.pack_split_axis;
  // Build the work queue (deprecated)
  // Keep the spot free for backwards compatibility
  pio.start_wq = 39;

  // num_pixels
  pio.mem[pio.start_wq + 0].ivalue = pio.image_width * pio.image_height;
  // cur_tile
  pio.mem[pio.start_wq + 1].ivalue = 0;
  pio.start_framebuffer = pio.start_wq + 2;
  pio.mem[7].ivalue  = pio.start_framebuffer; // offset for frame buffer ?
  int num_pixels     = pio.image_width * pio.image_height;
  const int channels = 3;
  int frame_size     = (num_pixels * channels);
  int start_zbuffer  = pio.start_framebuffer + frame_size;
  int zsize          = num_pixels;
  pio.mem[31].ivalue = start_zbuffer;
  for (int i = 0; i < zsize; ++i) {
    pio.mem[start_zbuffer + i].fvalue = 1000000;
  }
  pio.start_scene       = pio.start_framebuffer + frame_size + zsize;
  pio.start_permutation = pio.start_scene;
  
  //printf("FB starts at %d (0x%08x)\n", start_framebuffer, start_framebuffer);

  // set triangles saving edges rather than points before doing all the loads?
  Triangle::tri_stores_edges = pio.triangles_store_edges;
  
  std::vector<Triangle*> *triangles = new std::vector<Triangle*>();
  std::vector<Material*> *matls = new std::vector<Material*>();
  std::vector<PPM*> textures;

  // if no model specified, don't try to load the scene
  if(pio.model_file != NULL)
    {
      if (strstr(pio.model_file, ".objl")) {
        OBJListLoader::LoadModel(pio.model_file, triangles, matls, pio.mem, INT_MAX, pio.start_scene);
      } else if (strstr(pio.model_file, ".obj")) {
        OBJLoader::LoadModel(pio.model_file, triangles, matls, pio.mem, INT_MAX, pio.start_scene);
      } else if (strstr(pio.model_file, ".iw")) {
        IWLoader::LoadModel(pio.model_file, triangles, matls);
      } else {
        printf("ERROR: model file isn't .obj, .objl, or .iw\n");
        exit(-1);
      }

      pio.mem[29].ivalue = (int)triangles->size();

      // write materials to pio.memory
      pio.start_matls = pio.start_scene;
      // Add a grey material for objects without mat_id
      Material* grey = new Material(Vector3(0.78, 0.78, 0.78),
                                    Vector3(0.78, 0.78, 0.78),
                                    Vector3(0.78, 0.78, 0.78));
      matls->push_back(grey);

      //printf("Materials start at %d (0x%08x)\n", start_matls, start_matls);

      for (size_t i = 0; i < matls->size(); i++) {
        matls->at(i)->LoadIntoMemory(pio.start_scene, INT_MAX, pio.mem);
      }
      for (size_t i = 0; i < matls->size(); i++) {
        matls->at(i)->LoadTextureIntoMemory(pio.start_scene, INT_MAX, pio.mem);
      }

      //printf("Materials end at %d (0x%08x)\n", start_scene, start_scene);

      //might not need this
      pio.start_scene++;

      Grid* grid = NULL;
      if(pio.grid_dimensions==-1)
        pio.bvh = new BVH( triangles, pio.subtree_size, pio.duplicate_bvh, pio.triangles_store_edges, pio.pack_split_axis, pio.pack_stream_boundaries);
      else
        grid = new Grid(triangles, pio.triangles_store_edges, pio.grid_dimensions);
      pio.mem[21].ivalue = pio.bvh->num_nodes;
      int scene_data = pio.start_scene;

      printf("Scene starts at %d (0x%08x)\n", scene_data, scene_data);

      if(pio.grid_dimensions==-1)
        pio.bvh->LoadIntoMemory(scene_data, INT_MAX, pio.mem);
      else
        grid->LoadIntoMemory(scene_data, INT_MAX, pio.mem);

      pio.start_scene    = pio.bvh->start_nodes;
      pio.mem[8].ivalue  = pio.bvh->start_nodes;
      pio.mem[22].ivalue = pio.bvh->start_costs;
      pio.mem[24].ivalue = pio.bvh->start_secondary_nodes;
      pio.mem[26].ivalue = pio.bvh->start_subtree_sizes;
      pio.mem[27].ivalue = pio.bvh->start_rotated_flags;
      pio.mem[28].ivalue = pio.bvh->start_tris;
      pio.mem[30].ivalue = pio.bvh->start_tex_coords;
      pio.mem[32].ivalue = pio.bvh->start_parent_pointers;
      pio.mem[33].ivalue = pio.bvh->start_subtree_ids;
      pio.mem[34].ivalue = pio.bvh->num_subtrees;
      pio.mem[36].ivalue = pio.bvh->start_vertex_normals;
      pio.mem[37].ivalue = pio.bvh->num_interior_subtrees;
      

      //printf("Triangles start at %d (0x%08x)\n",bvh->start_tris, bvh->start_tris);
      
      // free up some pio.memory we don't need during the simulation (they've already been loaded)
      for(size_t i=0; i < triangles->size(); i++)
        delete triangles->at(i);
      triangles->clear();
      // do not delete the triangles vector, BVH still uses it

      //printf("Scene ends at %d (0x%08x)\n", scene_data, scene_data);

      pio.start_camera = scene_data + 1;

      int begin_camera = pio.start_camera;
      if(pio.camera != NULL)
      {
        //printf("Starting camera at %d (0x%08x)\n", begin_camera, begin_camera);
        pio.camera->LoadIntoMemory(begin_camera, INT_MAX, pio.mem);
        //printf("Camera ended at %d (0x%08x)\n", begin_camera, begin_camera);
      }
      pio.start_bg_color = begin_camera + 1;

      //printf("Background Color 0x%08x to 0x%08x\n", start_bg_color, start_bg_color+2);

      float bg_color[3] = {1.0f, 1.0f, 1.0f};
      for (int i = 0; i < 3; i++) {
        pio.mem[pio.start_bg_color + i].fvalue = bg_color[i];
      }

      pio.start_light = pio.start_bg_color + 3;

      //printf("Light at 0x%08x to 0x%08x\n", start_light, start_light+2);

      for (int i = 0; i < 3; i++) {
        pio.mem[pio.start_light + i].fvalue = pio.light_pos[i];
      }
      pio.start_permutation = pio.start_light + 3;
    } // end if(model_file != NULL)



  // add permutation table

  //printf("Permutation table from 0x%08x to 0x%08x\n", start_permutation,
  // start_permutation + 511);

  for (int i = 0; i < 256; i++) {
    pio.mem[pio.start_permutation + i].ivalue       = permutation[i];
    pio.mem[pio.start_permutation + i + 256].ivalue = permutation[i];
  }
  int start_hammersley = pio.start_permutation + 512;

  //printf("Hammersley table from 0x%08x to 0x%08x\n", start_hammersley,
  //	 start_hammersley + 511);

  for (int i = 0; i < 512; i++) {
    pio.mem[start_hammersley + i].fvalue = hammersley[i];
  }
  pio.end_memory = start_hammersley + 512;

  //printf("\npio.memory used: %d (0x%08x)\n", end_pio.memory, end_pio.memory);


  //printf("Image size: %d\n", image_size);
  //  printf("Tile pio.memory size: %d\n", num_tiles * 4);
  //  printf("pio.memory for everything else: %d\n", end_pio.memory - image_size - num_tiles * 4);

  // Pointers
  pio.mem[9].uvalue  = pio.start_matls;
  pio.mem[10].uvalue = pio.start_camera;
  pio.mem[11].uvalue = pio.start_bg_color;
  pio.mem[12].uvalue = pio.start_light;
  pio.mem[13].ivalue = pio.start_permutation;
  pio.mem[14].uvalue = pio.end_memory; // start free pio.memory
  pio.mem[15].uvalue = pio.mem_size-1; // end free pio.memory
  pio.mem[pio.end_memory].uvalue   = pio.end_memory+2; // initial head pointer for the queue
  pio.mem[pio.end_memory+1].uvalue = pio.end_memory+2; // initial tail pointer for the queue
  //pio.mem[16].ivalue = ray_depth
  //pio.mem[17].ivalue = num_samples
  //pio.mem[18].fvalue = epsilon
  pio.mem[19].ivalue = start_hammersley;
}
