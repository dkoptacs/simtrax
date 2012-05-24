#ifndef SIMHWRT_ANIMATION_H
#define SIMHWRT_ANIMATION_H

#include "Triangle.h"
#include "FourByte.h"
#include "OBJLoader.h"
#include <vector>
#include <cstdlib>
#include <string.h>

class Animation{
 public:
  char const *first_keyframe;
  int num_frames;
  FourByte *memory;
  int start_triangles;
  int start_seperate_triangles;
  int start_nodes;
  int start_seperate_nodes;
  int triangle_offset;
  int num_triangles;
  int num_nodes;
  int current_frame;
  int current_keyframe;
  int num_keyframes;
  char model_name[200];
  char next_filename[200];
  std::vector<Triangle*> current_triangles;
  std::vector<Triangle*> next_triangles;
  std::vector<int> *triangle_orders;
  std::vector<Material*> empty_mats;
  OBJLoader obj;
  float progress;
  float next_switch;
  float framestep;
  float keystep;
  float translate;
  int frames_this_phase;
  bool segregate_update;

  Animation(char const *keyframe, int n_frames, FourByte *mem, int n_nodes,
	    std::vector<int> *tri_orders, int num_tris, int start_tris, int start_nodes, int start_alt_tris = -1, int start_alt_nodes = -1);
  
  int loadNextFrame();
  void copyBVH();
  void getNextKeyFrame(char *name, int cur_framenum);
  int getRotateAddress()
  {
    return memory[24].ivalue;
  }
};

#endif
