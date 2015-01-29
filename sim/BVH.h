#ifndef _SIMHWRT_BVH_H_
#define _SIMHWRT_BVH_H_

#include "FourByte.h"
#include "Primitive.h"
#include <stdio.h>
#include <vector>
#include <queue>

class BVHNode;
namespace simtrax {
  class Triangle;
}


class BVH : public simtrax::Primitive {
public:
  ~BVH();
  BVH(std::vector<simtrax::Triangle*>* triangles, int _subtree_size, bool duplicate, bool tris_store_edges, bool pack_split_axis, bool _pack_stream_boundaries);

  void LoadIntoMemory(int &memory_position,
                      int max_memory,
                      FourByte* memory);
  void LoadNodes(int &memory_position,
		 int max_memory,
		 FourByte* memory, int start_node_copy);
  void LoadTriangles(int &memory_position,
		     int max_memory,
		     FourByte* memory);
  void LoadTextureCoords(int &memory_position,
			    int max_memory,
			    FourByte* memory);
  void LoadVertexNormals(int &memory_position,
			 int max_memory,
			 FourByte* memory);
  
  float oldComputeNodeCost(int nodeID);
  BVHNode loadNode(int nodeID, int start_scene, FourByte *memory);
  simtrax::Triangle loadTriangle(int tri_addr, FourByte *memory);
  float computeNodeCost(BVHNode node, int start_scene, FourByte *memory);
  float computeSAHCost(int start_scene, FourByte *memory);

  void writeDOT(const char *filename, int start_scene, FourByte *memory, int verbosity, int maxDepth = 32);
  void writeDOTNode(FILE *output, int nodeID, int start_scene, FourByte *memory, int verbosity, int depth, int maxDepth);

  void verifyTree(int start_scene, FourByte *memory, int nodeID);
  int verifyNodeCounts(int start_scene, FourByte *memory, int nodeID);
  int maxDepth(int start_scene, FourByte *memory, int nodeID);

  void build(int nodeID, int tri_begin, int tri_end,
             int& nextFree, int depth);
  void rebuild(FourByte *memory);
  void updateBounds(int ID);
  int computeSubtreeSize(int node_id);
  int assignSubtrees(int subtreeSize);
  int assignSingleSubtree(int subtreeRoot, int currentTreeID, int subtreeSize, std::queue<int>& global_q);
  void assignTriangleSubtrees();
  void assignTriangleSubtreesRecursive(int nodeID, int& remainingSpace);
  void reorderNodes();
 
#if 0
  void rotateNode(int id);
  void rotateTree(int id);
  void swapNodes(int i, int j);
#endif

  int start_costs;
  int start_subtree_sizes;
  int start_rotated_flags;
  int num_tris;
  int start_tris;
  int start_tex_coords;
  int start_vertex_normals;
  int start_textures;
  int start_secondary_tris;
  int start_nodes;
  int start_secondary_nodes;
  int start_parent_pointers;
  int start_subtree_ids;
  int num_subtrees;
  int num_interior_subtrees;
  int num_triangle_subtrees;
  int subtree_size;
  int triangle_subtree_size;
  int num_nodes;
  float initial_cost;
  bool duplicate_BVH;
  BVHNode* nodes;
  std::vector<simtrax::Triangle*>* triangles;
  std::vector<simtrax::Triangle*> inorder_tris;
  bool triangles_store_points;
  bool store_axis;
  bool pack_stream_boundaries;
  
  std::vector<int> tri_orders;

  int partitionSAH(int objBegin, int objEnd, int& output_axis);

  int getStartCosts()
  {
    return start_costs;
  }

  struct BVHCostEval
  {
    float position;
    float cost;
    int num_left;
    int num_right;
    int event;
    int axis;
  };

  struct BVHSAHEvent
  {
    float position;
    int obj_id;
    float left_area;
    float right_area;
    int num_left;
    int num_right;
    float cost;
  };

  struct CompareBVHSAHEvent
  {
    bool operator()(const BVHSAHEvent& x, const BVHSAHEvent& y)
    {
      // add obj_id sorting in here automatically?
      return x.position < y.position;
    }
  };

  bool buildEvents(int first,
                   int last,
                   int axis,
                   BVHCostEval& eval);

};

// tri_store_pts = true  -> triangles are normal (3 points for vertices)
// tri_store_pts = false -> triangles storing edge vectors (center pt + vectors to other 2)
void ExtendBoundByTriangle(float cur_min[3], float cur_max[3],
                           const simtrax::Triangle& tri, const bool &tri_store_pts=true );

void ExtendBoundByBox(float cur_min[3], float cur_max[3],
                      float box_min[3], float box_max[3]);

float BoxArea(const float box_min[3], const float box_max[3]);

int max(int a, int b);

#endif
