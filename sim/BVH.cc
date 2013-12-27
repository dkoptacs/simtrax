#include "BVH.h"
#include "Triangle.h"
#include <float.h>
#include <assert.h>
#include <limits.h>

#include <algorithm>

//#define TREE_ROTATIONS

const float BVH_C_isec = 1.f;
const float BVH_C_trav = 1.f;

// measured costs including all associated ops (load, stack, etc)
//const float BVH_C_isec = 146.92f;
//const float BVH_C_trav = 104.3f;


const int BVHNodeSize = 10;  // Box (6), start_child, num_children, parent, subtree
const int TriangleSize = 11; // 3 Verts (9), material_id, object_id (object ID for backwards compatibility only)

class BVHNode {
public:
  BVHNode(){}
  float box_min[3];
  float box_max[3];
  int start_child;  // child ID or direct block pointer for list of triangles
  char num_children; // -1 for interior nodes
  char split_axis;
  short triangle_subtree;

  int total_sub_nodes;
  float cost;
  int parent;
  int subtree;

  float area()
  {
    return BoxArea(box_min, box_max);  
  }
  
  void extendByNode(BVHNode n)
  {
    ExtendBoundByBox(box_min, box_max, n.box_min, n.box_max);
  }

  void setBounds(BVHNode n)
  {
    for(int i=0; i < 3; i++)
      {
	box_min[i] = n.box_min[i];
	box_max[i] = n.box_max[i];
      }

  }
  
  void makeLeaf(int first_child, int count) {
#ifdef TREE_ROTATIONS
    assert(count == 1);
#endif
    start_child = first_child;
    num_children = count;
  }

  void makeInternal(int first_child, int bestAxis) {
    start_child = first_child;
    num_children = -1;
    split_axis = bestAxis;
  }

  bool isLeaf() const {
    return num_children != -1;
  }

  void LoadIntoMemory(int &memory_position,
                      int max_memory,
                      FourByte* memory, bool store_axis, bool triangle_subtrees, bool pack_stream_boundaries) {
    // write myself into memory
    for (int i = 0; i < 3; i++) {
      memory[memory_position].fvalue = box_min[i];
      memory_position++;
    }
    for (int i = 0; i < 3; i++) {
      box_max[i] += 0.001f;  // don't allow for 2d boxes
      memory[memory_position].fvalue = box_max[i];
      memory_position++;
    }

    // Do leaf nodes need a pointer to the triangles' "treelet"?
    if(triangle_subtrees && num_children > 0)
      {
	int packedData = triangle_subtree & 0xffff;
	packedData <<= 16;
	packedData |= (num_children & 0xff);
	memory[memory_position].uvalue = packedData;
      }
    // Are we storing the split axis in one byte of num_children for interior nodes?
    else if(store_axis && num_children < 0)
      {
	int packedData = split_axis & 0xff;
	packedData <<= 8;
	packedData |= (num_children & 0xff);
	memory[memory_position].uvalue = packedData;
      }
    else
      {    
	memory[memory_position].ivalue = num_children;
      }
    memory_position++;

    memory[memory_position].ivalue = start_child;
    memory_position++;
  }

};

BVH::~BVH()
{
  delete[] nodes;
}

BVH::BVH(std::vector<Triangle*>* _triangles, int _subtree_size, bool duplicate, bool tris_store_edges, bool pack_split_axis, bool _pack_stream_boundaries) {
  subtree_size = _subtree_size;

  // TODO: Add a separate command line option for this. For now, just use the same size as the node treelets
  triangle_subtree_size = _subtree_size;

  triangles_store_points = !tris_store_edges;
  store_axis = pack_split_axis;
  pack_stream_boundaries = _pack_stream_boundaries;
  duplicate_BVH = duplicate;
  triangles = _triangles;
  num_tris = triangles->size();
  nodes = new BVHNode[2*triangles->size()];
  if(!nodes)
    {
      perror("in BVH::BVH: out of memory\n");
      exit(1);
    }
  int nextFree = 1;
  num_nodes = 0;
  build(0, 0, triangles->size(), nextFree, 0);
  updateBounds(0);
  oldComputeNodeCost(0);
  printf("before rotations, SAH cost = %f\n", nodes[0].cost);
  
  // not gonna get much more benefit after 10 without simulated annealing
  // Doesn't work with treelets
  //for(int i=0; i < 10; i++) 
  //rotateTree(0);
  
  //oldComputeNodeCost(0);
  //printf("after rotations, SAH cost = %f\n", nodes[0].cost);
  computeSubtreeSize(0);
  oldComputeNodeCost(0);

  num_subtrees = assignSubtrees(subtree_size);
  // If using subtrees, reorder the nodes so that each subtree is a contiguous block for direct-mapped caching
  if(num_subtrees > 0)
    {
      num_interior_subtrees = num_subtrees;
      printf("%d interior subtrees\n", num_interior_subtrees);
      reorderNodes();
      assignTriangleSubtrees();
      printf("assigned %d total subtrees\n", num_subtrees);
    }
}



void BVH::LoadIntoMemory(int &memory_position,
                         int max_memory,
                         FourByte* memory) {
  start_secondary_tris = -1;
  start_secondary_nodes = -1;
  printf("BVH bounds [%f %f %f] [%f %f %f]\n", 
	 nodes[0].box_min[0], nodes[0].box_min[1], nodes[0].box_min[2], 
	 nodes[0].box_max[0], nodes[0].box_max[1], nodes[0].box_max[2]);

  // align the BVH to cache line boundaries (root node starts mid-line, so that children are cache line-aligned)
  // want root node to be 8-word aligned and first child to be 32-word aligned
  memory_position = memory_position + (8 - (memory_position % 8));

  // load the first copy
  start_nodes = memory_position;
  LoadNodes(memory_position, max_memory, memory, start_nodes);
  start_tris = memory_position;
  LoadTriangles(memory_position, max_memory, memory);
  start_tex_coords = memory_position;
  LoadTextureCoords(memory_position, max_memory, memory);
  start_vertex_normals = memory_position;
  LoadVertexNormals(memory_position, INT_MAX, memory);

  // load the second copy (if necessary, for animations)
  if(duplicate_BVH)
    {
      memory_position = memory_position + (8 - (memory_position % 8)); // cache alignment
      start_secondary_nodes = memory_position;
      LoadNodes(memory_position, max_memory, memory, start_secondary_nodes);
      start_secondary_tris = memory_position;
      LoadTriangles(memory_position, max_memory, memory);
    }

  // Keep everything else separate from the actual BVH so that nodes remain 8-word aligned

  // now load the SAH costs for tree rotations
  start_costs = memory_position;
  for(int i = 0; i < num_nodes; i++)
    {
      if(nodes[i].num_children < 0)
	memory[memory_position++].fvalue = 0.0f;
      else
	memory[memory_position++].fvalue = nodes[i].num_children * BVH_C_isec;
    }

  // Now load the subtree sizes. The wording here is a bit confusing, this doesn't relate to treelets
  // This is a number that gives the size of the tree rooted at each node (for tree rotation thread assignment)
  start_subtree_sizes = memory_position;
  for(int i = 0; i < num_nodes; i++)
    {
      memory[memory_position++].ivalue = nodes[i].total_sub_nodes;
    }

  // now load the rotated flags
  // Note: this could be compressed down to 1 bit per node, but who cares
  //       It could even be hidden in one of the other node data fields, 
  //       like num_children
  start_rotated_flags = memory_position;
  for(int i = 0; i < num_nodes; i++)
    memory[memory_position++].ivalue = 0;


}

/* Loads every BVH node in to the simulator's memory */
void BVH::LoadNodes(int &memory_position,
		    int max_memory,
		    FourByte* memory, int start_node_copy)
{
  // Parent pointers will go right after the nodes, load them simultaneously
  start_parent_pointers = memory_position + (num_nodes * 8);
  // Subtree IDs will go right after that
  if(subtree_size > 0)
    start_subtree_ids = start_parent_pointers + num_nodes;
  else
    start_subtree_ids = -1; // Hopefully if the programmer tries to load this value, they will notice it's invalid
  
  for (int i = 0; i < num_nodes; i++) 
    {
      nodes[i].LoadIntoMemory(memory_position, max_memory, memory, store_axis, triangle_subtree_size > 0, pack_stream_boundaries);
      if(subtree_size > 0)
	{
	  memory[memory_position++].ivalue = nodes[i].parent;

	  // Store the parent and child subtree IDs instead of my own, so that during traversal, 
	  // I can switch streams without loading an address outside of the current stream's footprint
	  if(pack_stream_boundaries)
	    {
	      int packedData;
	      if(i == 0)
		packedData = 0 & 0xffff;
	      else
		packedData = nodes[nodes[i].parent].subtree & 0xffff;
	      //printf("packeddata1: %d\n", packedData);
	      packedData <<= 16;
	      //printf("packeddata2: %d\n", packedData);
	      if(!nodes[i].isLeaf())
		packedData |= (nodes[nodes[i].start_child].subtree & 0xffff);
	      memory[memory_position++].uvalue = packedData;
	      //printf("node %d, leaf = %d, parent subtree = %d, stream_boundaries = %d\n", i, nodes[i].isLeaf(), nodes[nodes[i].parent].subtree, packedData);
	    }
	  else
	    memory[memory_position++].ivalue = nodes[i].subtree;
	}
    }

  // after we've loaded all the nodes, go through and fix up the
  // memory references (in memory.. ugh)
  int node_size = subtree_size > 0 ? 10 : 8;
  int triangle_base_addr = memory_position;
  for (int i = 0; i < num_nodes; i++) 
    {
      if (nodes[i].isLeaf()) 
	{
	  // determine fixup and place in memory
	  int real_addr = triangle_base_addr + 11 * nodes[i].start_child; 
	  int node_addr = start_node_copy + i * node_size;
	  memory[node_addr + 7].ivalue = real_addr; 
	}
    }
}

void BVH::LoadTriangles(int &memory_position,
		   int max_memory,
		   FourByte* memory)
{
  for (size_t i = 0; i < inorder_tris.size(); i++) 
    {
      inorder_tris[i]->object_id = i; // fix up the object ID since they've been reordered
      inorder_tris[i]->LoadIntoMemory(memory_position, max_memory, memory);
    }
}

void BVH::LoadTextureCoords(int &memory_position,
			    int max_memory,
			    FourByte* memory)
{
  for (size_t i = 0; i < inorder_tris.size(); i++) 
    {
      inorder_tris[i]->LoadTextureCoords(memory_position, max_memory, memory);
    }
}

void BVH::LoadVertexNormals(int &memory_position,
			    int max_memory,
			    FourByte* memory)
{
  for (size_t i = 0; i < inorder_tris.size(); i++) 
    {
      inorder_tris[i]->LoadVertexNormals(memory_position, max_memory, memory);
    }
}

// Walks the tree and computes the number of children in 
// (including the node) each subtree
int BVH::computeSubtreeSize(int node_id)
{
  BVHNode *n = &nodes[node_id];
  if(n->num_children > 0) // leaf node
    {
      n->total_sub_nodes = 1;
      return 1;
    }
  n->total_sub_nodes = 1; // to account for myself
  n->total_sub_nodes += computeSubtreeSize(n->start_child); // left child
  n->total_sub_nodes += computeSubtreeSize(n->start_child + 1); // right child
  return n->total_sub_nodes;
}

/* 
   Groups the BVH in to subtrees. Assigns each node to exactly one subtree. 
   subtreeSize is size in words.
   returns number of subtrees assigned
   
   I really just need to read Timo's paper on how to build treelets. 
   For now though, I'm just going to get rid of the code that cuts off subtrees when they bottom out,
   even if they aren't full. This will result in subtrees having nodes that are potentially far apart.
   But I just need a temporary fix to cut the number of trees down for now.
*/
int BVH::assignSubtrees(int subtreeSize)
{
  if(subtree_size <= 0)
    return 0;

  int currentTreeID = 0;

  // Make a sentinel subtree so that we can use 1 and -1 as lastSubtree for the root (can't have -0).
  if(pack_stream_boundaries)
    currentTreeID++;

  int remainingTreeSpace;
  std::queue<int> global_q;

  // Make the top sub-tree
  remainingTreeSpace = assignSingleSubtree(0, currentTreeID, subtreeSize, global_q);

  // By now there should be some leftover nodes in the global queue
  while(!global_q.empty())
    {
      if(remainingTreeSpace < (2 * BVHNodeSize)) // need room for at least 2 nodes to keep siblings together
	{
	  currentTreeID++;
	  remainingTreeSpace = subtreeSize;
	}
      
      // Nodes will always come in pairs since we put siblings in the same subtree
      int leftNodeID = global_q.front();
      global_q.pop();
      int rightNodeID = global_q.front();
      global_q.pop();
      
      // Assign two half-trees to the same treelet ID (so that the sibling roots end up in the same treelet)
      int half_tree_size = remainingTreeSpace / 2;
      half_tree_size += assignSingleSubtree(leftNodeID, currentTreeID, half_tree_size, global_q);
      remainingTreeSpace = assignSingleSubtree(rightNodeID, currentTreeID, half_tree_size, global_q);      
    }

  return currentTreeID + 1;
}

// Does a breadth-first walk of the tree visiting nodes up to subteeSize in words, assigns them to currentTreeID
// Always puts siblings in to the same subtree
// Returns how much space is left in subtreeSize (may be > 0 if ran out of nodes)
int BVH::assignSingleSubtree(int subtreeRoot, int currentTreeID, int subtreeSize, std::queue<int>& global_q)
{
  std::queue<int> q;
  q.push(subtreeRoot); // Add root node to queue  
  
  int remainingTreeSpace = subtreeSize;
  bool fullTree = false;

  // Breadth first walk of the tree, putting up to subtreeSize words (plus spill-over for one sibling) in to each subtree
  while(!q.empty())
    {
      int nodeID = q.front();
      q.pop();
      // If the subtree isn't full, add this node to the subtree
      if(!fullTree)
	{
	  nodes[nodeID].subtree = currentTreeID;
	  // If it's an interior node, push its children
	  if(nodes[nodeID].num_children < 0)
	    {
	      q.push(nodes[nodeID].start_child); 
	      q.push(nodes[nodeID].start_child + 1);
	      remainingTreeSpace -= BVHNodeSize; 
	    }
	  else // leaf node, fill the subtree with node and triangles
	    {
	      remainingTreeSpace -= BVHNodeSize; // room for the node itself
	      
	      if(triangle_subtree_size < 0)
		remainingTreeSpace -= (nodes[nodeID].num_children * 11); // room for the triangles
	    }
	  // Siblings must always be assigned to the same subtree, unless they are the root of their own tree
	  // Subtrees must always end on a right child (even ID)
	  if(remainingTreeSpace <= 0 && (nodeID & 1) == 0)
	    {
	      fullTree = true;
	    }
	}
      else // This tree is full
	{
	  // put the remaining nodes on to the global queue to be assigned subtrees of their own
	  global_q.push(nodeID);
	}
    }
  return remainingTreeSpace < 0 ? 0 : remainingTreeSpace;
}


void BVH::assignTriangleSubtrees()
{
  if(triangle_subtree_size <= 0)
    return;
  int remainingSpace = triangle_subtree_size;
  assignTriangleSubtreesRecursive(0, remainingSpace);
  num_subtrees += 1;
}

void BVH::assignTriangleSubtreesRecursive(int nodeID, int& remainingSpace)
{
  if(nodes[nodeID].num_children < 0) // interior node
    {
      assignTriangleSubtreesRecursive(nodes[nodeID].start_child, remainingSpace);
      assignTriangleSubtreesRecursive(nodes[nodeID].start_child + 1, remainingSpace);  
    }
  else // leaf node
    {
      if(remainingSpace - (nodes[nodeID].num_children * TriangleSize) < 0)
	{
	  // create a new triangle subtree
	  remainingSpace = triangle_subtree_size;
	  num_subtrees++;
	}
      
      remainingSpace -= (nodes[nodeID].num_children * TriangleSize);
      nodes[nodeID].triangle_subtree = num_subtrees;
      //printf("(node %d): assigned %d triangles at %d to treelet %d\n", nodeID, nodes[nodeID].num_children, nodes[nodeID].start_child, num_subtrees);
    }
}

// Rearranges nodes so that each treelet is a contiguous block of memory for direct-mapped caching
void BVH::reorderNodes()
{
  // Will hold the IDs of nodes in order of treelet ID
  std::vector<int>* ids_by_treelet = new std::vector<int>[num_subtrees];
  if(!ids_by_treelet)
    {
      printf("Malloc failed in reorderNodes\n");
      exit(1);
    }

  // Put ids in to vector corresponding to its tree id
  // Since siblings are already next to each other in nodes[], and they are required to be in the same
  // subtree, going through nodes[] in linear order will do the trick
  
  for(int i = 0; i < num_nodes; i++)
    {
      ids_by_treelet[nodes[i].subtree].push_back(i);
    }
  

  // Now fix up the child pointers
  int k = 0;
  for(int i = 0; i < num_subtrees; i++)
    for(size_t j = 0; j < ids_by_treelet[i].size(); j++)
      {
	if((ids_by_treelet[i].at(j) % 2) != 0) // only left children are pointed to
	  {
	    nodes[nodes[ids_by_treelet[i].at(j)].parent].start_child = k;
	  }
	k++;
      }

  // Now and copy them over to a new array 
  BVHNode* reordered = new BVHNode[num_nodes];
  if(!reordered)
    {
      printf("Malloc failed in reorderNodes\n");
      exit(1);
    }

  k = 0;
  for(int i = 0; i < num_subtrees; i++)
    for(size_t j = 0; j < ids_by_treelet[i].size(); j++)
      reordered[k++] = nodes[ids_by_treelet[i].at(j)];

  // Now fix up the parent pointers
  for(int i = 0; i < num_nodes; i++)
    {
      if(reordered[i].num_children >= 0)
	continue;
      reordered[reordered[i].start_child + 0].parent = i;
      reordered[reordered[i].start_child + 1].parent = i;
    }
  
  // Now set the new pointer
  delete[] nodes;
  delete[] ids_by_treelet;
  nodes = reordered;

}


float BoxArea(const float box_min[3], const float box_max[3]) {
  float dx = box_max[0] - box_min[0];
  float dy = box_max[1] - box_min[1];
  float dz = box_max[2] - box_min[2];

  return 2.f * (dx * dy + dx * dz + dy * dz);
}

// dot -Tgif initialBVH.dot -o out.gif
void BVH::writeDOT(const char *filename, int start_scene, FourByte *memory, int verbosity, int maxDepth)
{
  FILE *output = fopen(filename, "w");
  if(output == NULL)
    {
      printf("Error, could not open DOT file for writing: %s\n", filename);
      exit(1);
    }
  // BVHNode root = loadNode(0, start_scene, memory);
  fprintf(output, "graph BVH {\n");
  //fprintf(output, "Node0 [label=\"0:[%f %f %f]-[%f %f %f](%f),1, I\"];\n", 
  //  root.box_min[0], root.box_min[1], root.box_min[2],
  //  root.box_max[0], root.box_max[1], root.box_max[2],
  //  memory[start_costs].fvalue);
  unsigned char red = 127;
  unsigned char green = 127;
  BVHNode root = loadNode(0, start_scene, memory);
//   char isLeaf = root.num_children >=0 ? 'L' : 'I';
  if(verbosity)
    fprintf(output, "Node0 [label=\"0, %d, %d\"];\n", root.start_child, root.subtree);
  else
    {
      fprintf(output, "ratio=0.5;\n");
      float current_cost = memory[start_costs + 0].fvalue;
      float delta = current_cost / root.cost;
      red = (unsigned char)((delta/2.0) * 255);
      green = 255 - red;
      fprintf(output, "Node0 [label=\"\" fixedsize=true width=0.0 height=0.0 color=\"#%02x%02x00\"];\n", red, green);
    }
  writeDOTNode(output, 0, start_scene, memory, verbosity, 0, maxDepth);
  fprintf(output, "}\n");
  fclose(output);
}

// assumes the node itself has already been written, 
// actually writes children nodes and the links to them
void BVH::writeDOTNode(FILE *output, int nodeID, int start_scene, FourByte *memory, int verbosity, int depth, int maxDepth)
{
  if(depth == maxDepth)
    return;
  BVHNode node = loadNode(nodeID, start_scene, memory);

  if(node.num_children >= 0)
    return;
  BVHNode left = loadNode(node.start_child, start_scene, memory);
  BVHNode right = loadNode(node.start_child+1, start_scene, memory);
//   char isLLeaf = left.num_children >=0 ? 'L' : 'I';
//   char isRLeaf = right.num_children >=0 ? 'L' : 'I';


  // char leftClass = 'L', rightClass = 'L';
  // if(left.num_children < 0)
  //   leftClass = 'I';
  // if(right.num_children < 0)
  //   rightClass = 'I';
  
  float current_cost, delta;
  unsigned char red, green; 
  if(verbosity)
    {
      if(pack_stream_boundaries)
	{
	  fprintf(output, "Node%d [label=\"%d, %d, (%d, %d)\"];\n", node.start_child, node.start_child, left.start_child, (left.subtree & 0xffff0000) >> 16, (left.subtree & 0xffff));
	  fprintf(output, "Node%d [label=\"%d, %d, (%d, %d)\"];\n", node.start_child + 1, node.start_child + 1, right.start_child, (right.subtree & 0xffff0000) >> 16, (right.subtree & 0xffff));
	}
      else
	{
	  fprintf(output, "Node%d [label=\"%d, %d, %d\"];\n", node.start_child, node.start_child, left.start_child, left.subtree);
	  fprintf(output, "Node%d [label=\"%d, %d, %d\"];\n", node.start_child + 1, node.start_child + 1, right.start_child, right.subtree);
	}
      fprintf(output, "Node%d -- Node%d;\n", nodeID, node.start_child);
      fprintf(output, "Node%d -- Node%d;\n", nodeID, node.start_child+1);
    }
  else
    {
      current_cost = memory[start_costs + node.start_child].fvalue;
      delta = current_cost / nodes[node.start_child].cost; // compare to the original tree
      red = (unsigned char)((delta/2.0) * 255);
      green = 255 - red;
      fprintf(output, "Node%d [label=\"\" fixedsize=true width=0.0 height=0.0 color=\"#%02x%02x00\"];\n", node.start_child, red, green);
      fprintf(output, "Node%d -- Node%d [color=\"#%02x%02x00\" weight=100];\n", nodeID, node.start_child, red, green);

      current_cost = memory[start_costs + node.start_child+1].fvalue;
      delta = current_cost / nodes[node.start_child + 1].cost; // compare to the original tree
      red = (unsigned char)((delta/2.0) * 255);
      green = 255 - red;
      fprintf(output, "Node%d [label=\"\" fixedsize=true width=0.0 height=0.0 color=\"#%02x%02x00\"];\n", node.start_child+1, red, green);
      fprintf(output, "Node%d -- Node%d [color=\"#%02x%02x00\" weight=100];\n", nodeID, node.start_child+1, red, green);
    }
  
  writeDOTNode(output, node.start_child, start_scene, memory, verbosity, depth + 1, maxDepth);
  writeDOTNode(output, node.start_child+1, start_scene, memory, verbosity, depth + 1, maxDepth);
}

Triangle BVH::loadTriangle(int tri_addr, FourByte *memory)
{
  Triangle t;
  for(int i=0; i < 3; i++)
    {
      t.p0[i] = memory[tri_addr + i + 0].fvalue;
      t.p1[i] = memory[tri_addr + i + 3].fvalue;
      t.p2[i] = memory[tri_addr + i + 6].fvalue;
    }
  return t;
}

int max(int a, int b)
{
  return a > b ? a : b;
}

int BVH::maxDepth(int start_scene, FourByte *memory, int nodeID)
{
  BVHNode n = loadNode(nodeID, start_scene, memory);
  if(n.num_children > 0) // leaf node
    return 1;
  return 1 + max(maxDepth(start_scene, memory, n.start_child), maxDepth(start_scene, memory, n.start_child + 1));
}

void BVH::verifyTree(int start_scene, FourByte *memory, int nodeID)
{
  if(nodeID < 0 || nodeID >= num_nodes)
    printf("Invalid nodeID: %d\n", nodeID);
  if(memory[start_rotated_flags + nodeID].ivalue != 0)
    printf("Node %d, is_rotated flag should be off\n", nodeID);
  BVHNode n = loadNode(nodeID, start_scene, memory);
  float mins[3] = {1000000.0f, 1000000.0f, 1000000.0f};
  float maxs[3] = {-1000000.0f, -1000000.0f, -1000000.0f};
  bool isleaf = (memory[start_scene + (nodeID * 8) + 6].ivalue > 0 );
  if(isleaf)
    {
      Triangle t = loadTriangle(memory[start_scene + (nodeID * 8) + 7].ivalue,
				memory);
	  ExtendBoundByTriangle(mins, maxs, t, triangles_store_points );
    }
  else
    {
      BVHNode left = loadNode(memory[start_scene + (nodeID * 8) + 7].ivalue, start_scene, memory);
      BVHNode right = loadNode(memory[start_scene + (nodeID * 8) + 7].ivalue + 1, start_scene, memory);
      ExtendBoundByBox(mins, maxs, left.box_min, left.box_max);
      ExtendBoundByBox(mins, maxs, right.box_min, right.box_max);
    }
  
  for(int i=0; i < 3; i++)
    {
      if((n.box_max[i] < maxs[i]) || (n.box_min[i] > mins[i]))
	{
	  if(isleaf)
	    printf("invalid leaf node: ");
	  else
	    printf("invalid inner node: ");
	  printf("%d\tchild bounds = [%f %f %f][%f %f %f], parent bounds = [%f %f %f][%f %f %f]\n",
		 nodeID, mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2],
		 n.box_min[0], n.box_min[1], n.box_min[2], n.box_max[0], n.box_max[1], n.box_max[2]);
	  break;
	}
    }
  if(isleaf) 
    return;
  verifyTree(start_scene, memory, memory[start_scene + (nodeID * 8) + 7].ivalue);
  verifyTree(start_scene, memory, memory[start_scene + (nodeID * 8) + 7].ivalue + 1);
  //if(nodeID == 0)
  //verifyNodeCounts(start_scene, memory, nodeID);
}

int BVH::verifyNodeCounts(int start_scene, FourByte *memory, int nodeID)
{
  BVHNode n = loadNode(nodeID, start_scene, memory);
  if(n.num_children > 0) // leaf
    {
      if(n.total_sub_nodes != 1)
	printf("Invalid leaf node: %d, subtree size = %d\n", nodeID, n.total_sub_nodes);
      return 1;
    }
  int left_size = verifyNodeCounts(start_scene, memory, n.start_child);
  int right_size = verifyNodeCounts(start_scene, memory, n.start_child + 1);

  if(n.total_sub_nodes != (left_size + right_size + 1))
    printf("Invalid inner node: %d, subtree size = %d, should be %d\n", nodeID, n.total_sub_nodes, left_size + right_size + 1);

  return left_size + right_size + 1;
}

BVHNode BVH::loadNode(int nodeID, int start_scene, FourByte *memory)
{
  BVHNode retVal;
  int node_size = subtree_size > 0 ? 10 : 8;
  int node_addr = start_scene + nodeID * node_size;
  for(int i=0; i < 3; i++)
    {
      retVal.box_min[i] = memory[node_addr + i].fvalue;
      retVal.box_max[i] = memory[node_addr + i+3].fvalue;
    }
  retVal.num_children = memory[node_addr + 6].ivalue;  
  retVal.start_child = memory[node_addr + 7].ivalue;
  retVal.total_sub_nodes = memory[start_subtree_sizes + nodeID].ivalue;
  retVal.cost = memory[start_costs + nodeID].fvalue;
  retVal.parent = memory[start_parent_pointers + nodeID].ivalue;
  if(subtree_size > 0)
    retVal.subtree = memory[node_addr + 9].ivalue;
  else
    retVal.subtree = 0;
  
  return retVal;
}


float BVH::oldComputeNodeCost(int nodeID)
{
  BVHNode *node = &nodes[nodeID];
  if(node->num_children >= 0) // leaf node
    return node->num_children * BVH_C_isec;
  else
    {
      const int leftID = node->start_child;
      const int rightID = leftID + 1;
      float leftCost = oldComputeNodeCost(leftID);
      float rightCost = oldComputeNodeCost(rightID);
      float area = BoxArea(node->box_min, node->box_max);
      float leftArea = BoxArea(nodes[leftID].box_min, nodes[leftID].box_max);
      float rightArea = BoxArea(nodes[rightID].box_min, nodes[rightID].box_max);
      node->cost = BVH_C_trav + (leftArea * leftCost + 
				 rightArea * rightCost) / area;
      return node->cost;
      // TODO: this divide cancels out the multiply by areas on the next level up
      // can be taken out, but this isn't part of the simulation, 
      // so doesn't really matter
    }
}

float BVH::computeNodeCost(BVHNode node, int start_scene, FourByte *memory)
{
  if(node.num_children > 0)
    return node.num_children * BVH_C_isec;

  BVHNode left = loadNode(node.start_child, start_scene, memory);
  BVHNode right = loadNode(node.start_child+1, start_scene, memory);

  float leftCost = computeNodeCost(left, start_scene, memory);
  float rightCost = computeNodeCost(right, start_scene, memory);

  float area = BoxArea(node.box_min, node.box_max);

  float leftArea = BoxArea(left.box_min, left.box_max);
  float rightArea = BoxArea(right.box_min, right.box_max);
  return BVH_C_trav + (leftArea * leftCost + 
		       rightArea * rightCost) / area;
  
  // TODO: this divide cancels out the multiply by areas on the next level up
  // can be taken out, but this isn't part of the simulation, 
  // so doesn't really matter
  
}


float BVH::computeSAHCost(int start_scene, FourByte *memory)
{
  BVHNode root = loadNode(0, start_scene, memory);
  float retVal = computeNodeCost(root, start_scene, memory);
  return retVal;
}

int partitionObjectMedian(int tri_begin, int tri_end, int bestAxis) {
  int num_tris = tri_end - tri_begin;
  if (num_tris <= 4) {
    bestAxis = -1;
    return -1;
  } else {
    bestAxis = 0;
    return tri_begin + num_tris/2;
  }

}

void BVH::rebuild(FourByte *memory)
{
  // delete and remove the old triangles
  for(size_t i = 0; i < triangles->size(); i++)
    delete triangles->at(i);
    
  inorder_tris.clear();
  triangles->clear();

  // get the new triangles form the simulator's memory
  for(int i = 0; i < num_tris; i++)
    {
      Triangle t = loadTriangle(start_tris + (i*11), memory);
      triangles->push_back(new Triangle(t));
    }
  
  int nextFree = 1;
  num_nodes = 0;
  build(0, 0, triangles->size(), nextFree, 0);
  updateBounds(0);
  computeSubtreeSize(0);
  oldComputeNodeCost(0);

  int memory_position = start_nodes;
  LoadNodes(memory_position, INT_MAX, memory, start_nodes);

  start_tris = memory_position;
  LoadTriangles(memory_position, INT_MAX, memory);
  start_tex_coords = memory_position;
  LoadTextureCoords(memory_position, INT_MAX, memory);
  start_vertex_normals = memory_position;
  LoadVertexNormals(memory_position, INT_MAX, memory);
}

void BVH::build(int nodeID, int tri_begin, int tri_end,
                int& nextFree, int depth) {
  if (depth == 0) {
    printf("Starting BVH build.\n");
  }

  assert (tri_end > tri_begin);

  BVHNode& node = nodes[nodeID];
  if(nodeID == 0) // set root node's parent to invalid
    node.parent = -1;

  int num_tris = tri_end - tri_begin;
  num_nodes++;
  int bestAxis = -1;
  //int split = partitionObjectMedian(tri_begin, tri_end, bestAxis);
  int split = partitionSAH(tri_begin, tri_end, bestAxis);

  if (bestAxis == -1) {
    // make leaf
    int first_inorder = int(inorder_tris.size());
    for (int i = 0; i < num_tris; i++) {
      inorder_tris.push_back(triangles->at(tri_begin + i));
    }
    node.makeLeaf(first_inorder, num_tris);
  }
  else {
    //int split = tri_begin + num_tris/2;
    // make internal node
    node.makeInternal(nextFree, bestAxis);
    nextFree += 2;

    // Set children nodes' parent to current node
    nodes[node.start_child].parent = nodeID;
    nodes[node.start_child + 1].parent = nodeID;

    build(node.start_child + 0,tri_begin,split,nextFree,depth+1);
    build(node.start_child + 1,split,tri_end,nextFree,depth+1);
  }

  if (depth > 63) printf("Uh oh, BVH depth just hit %d\n", depth);
  
  if (depth == 0) {
    for(size_t i=0; i < inorder_tris.size(); i++)      
      tri_orders.push_back(inorder_tris[i]->object_id); 
    printf("BVH build complete with %d nodes.\n", num_nodes);
  }
}

void ResetBound(float min[3], float max[3]) {
  for (int i = 0; i < 3; i++) {
    min[i] = FLT_MAX;
    max[i] = -FLT_MAX;
  }
}

void ExtendBoundByBox(float cur_min[3], float cur_max[3],
                      float box_min[3], float box_max[3]) {
  for (int i = 0; i < 3; i++) {
    if (box_min[i] < cur_min[i])
      cur_min[i] = box_min[i];
    if (box_max[i] > cur_max[i])
      cur_max[i] = box_max[i];
  }
}

void ExtendBoundByTriangle(float cur_min[3], float cur_max[3],
                           const Triangle& tri, const bool &tri_store_pts ) {

  if( tri_store_pts ) {
	  for (int i = 0; i < 3; i++) {
		if (tri.p0[i] < cur_min[i]) {
		  cur_min[i] = static_cast<float>( tri.p0[i] );
		}
		if (tri.p1[i] < cur_min[i]) {
		  cur_min[i] = static_cast<float>( tri.p1[i] );
		}
		if (tri.p2[i] < cur_min[i]) {
		  cur_min[i] = static_cast<float>( tri.p2[i] );
		}

		if (tri.p0[i] > cur_max[i]) {
		  cur_max[i] = static_cast<float>( tri.p0[i] );
		}
		if (tri.p1[i] > cur_max[i]) {
		  cur_max[i] = static_cast<float>( tri.p1[i] );
		}
		if (tri.p2[i] > cur_max[i]) {
		  cur_max[i] = static_cast<float>( tri.p2[i] );
		}
	  }
  }
  else {
	 for (int i = 0; i < 3; i++) {
		const float tmp0 = static_cast<float>( tri.p0[i] + tri.p1[i] );
		if (tmp0 < cur_min[i]) {
		  cur_min[i] = tmp0;
		}
		if (tmp0 > cur_max[i]) {
		  cur_max[i] = tmp0;
		}

		if (tri.p1[i] < cur_min[i]) {
		  cur_min[i] = static_cast<float>( tri.p1[i] );
		}
		if (tri.p1[i] > cur_max[i]) {
		  cur_max[i] = static_cast<float>( tri.p1[i] );
		}

		const float tmp2 = static_cast<float>( tri.p2[i] + tri.p1[i] );
		if (tmp2 < cur_min[i]) {
		  cur_min[i] = tmp2;
		}
		if (tmp2 > cur_max[i]) {
		  cur_max[i] = tmp2;
		}
	  }
  }
}


void BVH::updateBounds(int ID) {
  BVHNode& node = nodes[ID];
  if (node.isLeaf()) {
    ResetBound(node.box_min, node.box_max);
    for (int i = 0; i < node.num_children; i++) {
      int child_id = node.start_child + i;
      ExtendBoundByTriangle(node.box_min,
                            node.box_max,
                            *inorder_tris[child_id],
							triangles_store_points);
    }
  } else {
    int left_son = node.start_child;
    int right_son = left_son + 1;
    updateBounds(left_son);
    updateBounds(right_son);

    ResetBound(node.box_min, node.box_max);
    BVHNode& left_node = nodes[left_son];
    BVHNode& right_node = nodes[right_son];

    ExtendBoundByBox(node.box_min, node.box_max,
                     left_node.box_min, left_node.box_max);
    ExtendBoundByBox(node.box_min, node.box_max,
                     right_node.box_min, right_node.box_max);
  }
}

int BVH::partitionSAH(int objBegin, int objEnd, int& output_axis)
{
    int num_objects = objEnd - objBegin;
    if ( num_objects == 1 )
    {
        output_axis = -1;
        return -1;
    }

    BVHCostEval best_cost;
    best_cost.event = 0;
#ifdef TREE_ROTATIONS
    best_cost.cost = FLT_MAX;
#else
    best_cost.cost = BVH_C_isec * num_objects;
#endif
    best_cost.axis = -1;

    for ( int axis = 0; axis < 3; axis++ )
    {
        BVHCostEval new_cost;
        if ( buildEvents(objBegin,objEnd,axis,new_cost) )
        {
            if ( new_cost.cost < best_cost.cost )
            {
                best_cost = new_cost;
            }
        }
    }

    output_axis = best_cost.axis;
    if ( output_axis != -1 )
    {
        // build the events and sort them
        std::vector<BVHSAHEvent> events;
        for ( int i = objBegin; i < objEnd; i++ )
        {
          float tri_box_min[3];
          float tri_box_max[3];
          ResetBound(tri_box_min, tri_box_max);
          ExtendBoundByTriangle(tri_box_min, tri_box_max,
                                *triangles->at(i), triangles_store_points);

          BVHSAHEvent new_event;
          new_event.position = .5f*(tri_box_min[output_axis] +
                                    tri_box_max[output_axis]);
          new_event.obj_id   = i;
          events.push_back(new_event);
        }
        std::sort(events.begin(), events.end(), CompareBVHSAHEvent());
        std::vector<Triangle*> copied_tris;
        for (size_t i = 0; i < events.size(); i++) {
          copied_tris.push_back(triangles->at(events.at(i).obj_id));
        }

        for (int i = objBegin; i < objEnd; i++) {
          triangles->at(i) = copied_tris.at(i-objBegin);
        }

        int result = objBegin + best_cost.event;

        if ( result == objBegin || result == objEnd )
        {
            if ( num_objects < 8 )
            {
                output_axis = -1;
                return 0;
            }
            result = objBegin + num_objects/2;
            return result;
        }
        else
            return result;
    }
    else
    {
        return 0; // making a leaf anyway
    }

}

bool BVH::buildEvents(int first,
                      int last,
                      int axis,
                      BVHCostEval& best_eval)
{
  float overall_box_min[3];
  float overall_box_max[3];
  ResetBound(overall_box_min, overall_box_max);

  std::vector<BVHSAHEvent> events;
  for ( int i = first; i < last; i++ )
  {
    BVHSAHEvent new_event;
    float tri_box_min[3];
    float tri_box_max[3];
    ResetBound(tri_box_min, tri_box_max);
    ExtendBoundByTriangle(tri_box_min, tri_box_max, *triangles->at(i), triangles_store_points);

    new_event.position = .5f * (tri_box_min[axis] + tri_box_max[axis]);
    new_event.obj_id   = i;
    events.push_back(new_event);

    ExtendBoundByBox(overall_box_min, overall_box_max,
                     tri_box_min, tri_box_max);
  }

  std::sort(events.begin(),events.end(),CompareBVHSAHEvent());

  int num_events = int(events.size());
  float left_box_min[3];
  float left_box_max[3];
  ResetBound(left_box_min, left_box_max);

  int num_left = 0;
  int num_right = num_events;

    for ( size_t i = 0; i < events.size(); i++ )
    {
        events[i].num_left = num_left;
        events[i].num_right = num_right;
        events[i].left_area = BoxArea(left_box_min, left_box_max);

        float tri_box_min[3];
        float tri_box_max[3];
        ResetBound(tri_box_min, tri_box_max);
        ExtendBoundByTriangle(tri_box_min, tri_box_max,
                              *triangles->at(events[i].obj_id), triangles_store_points);
        ExtendBoundByBox(left_box_min, left_box_max,
                         tri_box_min, tri_box_max);

        num_left++;
        num_right--;
    }

    float right_box_min[3];
    float right_box_max[3];
    ResetBound(right_box_min, right_box_max);

    best_eval.cost = FLT_MAX;
    best_eval.event = -1;

    for ( int i = num_events - 1; i >= 0; i-- )
    {
        float tri_box_min[3];
        float tri_box_max[3];
        ResetBound(tri_box_min, tri_box_max);
        ExtendBoundByTriangle(tri_box_min, tri_box_max,
                              *triangles->at(events[i].obj_id), triangles_store_points);
        ExtendBoundByBox(right_box_min, right_box_max,
                         tri_box_min, tri_box_max);

        if ( events[i].num_left > 0 && events[i].num_right > 0 )
        {
            events[i].right_area = BoxArea(right_box_min,
                                           right_box_max);

            float this_cost = (events[i].num_left * events[i].left_area +
                               events[i].num_right * events[i].right_area);
            this_cost /= BoxArea(overall_box_min,
                                 overall_box_max);
            this_cost *= BVH_C_isec;
            this_cost += BVH_C_trav;

            events[i].cost = this_cost;
            if ( this_cost < best_eval.cost )
            {
                best_eval.cost       = this_cost;
                best_eval.position   = events[i].position;
                best_eval.axis       = axis;
                best_eval.event      = i;
                best_eval.num_left   = events[i].num_left;
                best_eval.num_right  = events[i].num_right;

            }
        }
    }
    return best_eval.event != -1;
}

#if 0
void BVH::rotateNode(int nodeID)
{
  const int leftID = nodes[ nodeID ].start_child;
  const int rightID = nodes[ nodeID ].start_child + 1;
  const int leftLeftID = nodes[ leftID ].start_child;
  const int leftRightID = nodes[ leftID ].start_child + 1;
  const int rightLeftID = nodes[ rightID ].start_child;
  const int rightRightID = nodes[ rightID ].start_child + 1;
  float nodeArea = nodes[ nodeID ].area();
  float leftArea = nodes[ leftID ].area();
  float rightArea = nodes[ rightID ].area();
  float leftLeftArea = 0.0;
  float leftRightArea = 0.0;
  float rightLeftArea = 0.0;
  float rightRightArea = 0.0;
  BVHNode lowerBoxA;
  BVHNode lowerBoxB;
  float lowerAreaA = 0.0;
  float lowerAreaB = 0.0;
  int best = 0;
  float bestCost = ( nodeArea * BVH_C_trav +
                    leftArea * nodes[ leftID ].cost +
                    rightArea * nodes[ rightID ].cost );
  // Step 1: Which rotation reduces the cost of this node the most?
  if ( !nodes[ leftID ].isLeaf() )
  {
    leftLeftArea = nodes[ leftLeftID ].area();
    leftRightArea = nodes[ leftRightID ].area();
    float partialCost = ( nodeArea * BVH_C_trav +
                         rightArea * nodes[ rightID ].cost +
                         leftRightArea * nodes[ leftRightID ].cost +
                         leftLeftArea * nodes[ leftLeftID ].cost );
    // (1)    N                     N      //
    //       / \                   / \     //
    //      L   R     ----->      L   LL   //
    //     / \                   / \       //
    //   LL   LR                R   LR     //
    BVHNode lowerBox1 = nodes[ rightID ];
    lowerBox1.extendByNode( nodes[ leftRightID ] );
    float lowerArea1 = lowerBox1.area();
    float upperCost1 = lowerArea1 * BVH_C_trav + partialCost;
    if ( upperCost1 < bestCost )
    {
      best = 1;
      bestCost = upperCost1;
      lowerBoxA = lowerBox1;
      lowerAreaA = lowerArea1;
    }
    // (2)    N                     N      //
    //       / \                   / \     //
    //      L   R     ----->      L   LR   //
    //     / \                   / \       //
    //   LL   LR               LL   R      //
    BVHNode lowerBox2 = nodes[ rightID ];
    lowerBox2.extendByNode( nodes[ leftLeftID ] );
    float lowerArea2 = lowerBox2.area();
    float upperCost2 = lowerArea2 * BVH_C_trav + partialCost;
    if ( upperCost2 < bestCost )
    {
      best = 2;
      bestCost = upperCost2;
      lowerBoxA = lowerBox2;
      lowerAreaA = lowerArea2;
    }
  }
  if ( !nodes[ rightID ].isLeaf() )
  {
    rightLeftArea = nodes[ rightLeftID ].area();
    rightRightArea = nodes[ rightRightID ].area();
    float partialCost = ( nodeArea * BVH_C_trav +
                         leftArea * nodes[ leftID ].cost +
                         rightRightArea * nodes[ rightRightID ].cost +
                         rightLeftArea * nodes[ rightLeftID ].cost );
    // (3)    N                     N        //
    //       / \                   / \       //
    //      L   R     ----->     RL   R      //
    //         / \                   / \     //
    //       RL   RR                L   RR   //
    BVHNode lowerBox3 = nodes[ leftID ];
    lowerBox3.extendByNode( nodes[ rightRightID ] );
    float lowerArea3 = lowerBox3.area();
    float upperCost3 = lowerArea3 * BVH_C_trav + partialCost;
    if ( upperCost3 < bestCost )
    {
      best = 3;
      bestCost = upperCost3;
      lowerBoxA = lowerBox3;
      lowerAreaA = lowerArea3;
    }
    // (4)    N                     N       //
    //       / \                   / \      //
    //      L   R     ----->     RR   R     //
    //         / \                   / \    //
    //       RL   RR               RL   L   //
    BVHNode lowerBox4 = nodes[ leftID ];
    lowerBox4.extendByNode( nodes[ rightLeftID ] );
    float lowerArea4 = lowerBox4.area();
    float upperCost4 = lowerArea4 * BVH_C_trav + partialCost;
    if ( upperCost4 < bestCost )
    {
      best = 4;
      bestCost = upperCost4;
      lowerBoxA = lowerBox4;
      lowerAreaA = lowerArea4;
    }
    if ( !nodes[ leftID ].isLeaf() )
    {
      float partialCost = ( nodeArea * BVH_C_trav +
                           leftLeftArea * nodes[ leftLeftID ].cost +
                           leftRightArea * nodes[ leftRightID ].cost +
                           rightLeftArea * nodes[ rightLeftID ].cost +
                           rightRightArea * nodes[ rightRightID ].cost );
      // (5)       N                      N        //
      //         /   \                  /   \      //
      //        L     R     ----->     L     R     //
      //       / \   / \              / \   / \    //
      //      LL LR RL RR            LL RL LR RR   //
      BVHNode lowerBox5L = nodes[ leftLeftID ];
      lowerBox5L.extendByNode( nodes[ rightLeftID ] );
      float lowerArea5L = lowerBox5L.area();
      BVHNode lowerBox5R = nodes[ leftRightID ];
      lowerBox5R.extendByNode( nodes[ rightRightID ] );
      float lowerArea5R = lowerBox5R.area();
      float upperCost5 = ( lowerArea5L + lowerArea5R ) * BVH_C_trav + partialCost;
      if ( upperCost5 < bestCost )
      {
        best = 5;
        bestCost = upperCost5;
        lowerBoxA = lowerBox5L;
        lowerAreaA = lowerArea5L;
        lowerBoxB = lowerBox5R;
        lowerAreaB = lowerArea5R;
      }
      // (6)       N                      N        //
      //         /   \                  /   \      //
      //        L     R     ----->     L     R     //
      //       / \   / \              / \   / \    //
      //      LL LR RL RR            LL RR RL LR   //
      BVHNode lowerBox6L = nodes[ leftLeftID ];
      lowerBox6L.extendByNode( nodes[ rightRightID ] );
      float lowerArea6L = lowerBox6L.area();
      BVHNode lowerBox6R = nodes[ rightLeftID ];
      lowerBox6R.extendByNode( nodes[ leftRightID ] );
      float lowerArea6R = lowerBox6R.area();
      float upperCost6 = ( lowerArea6L + lowerArea6R ) * BVH_C_trav + partialCost;
      if ( upperCost6 < bestCost )
      {
        best = 6;
        bestCost = upperCost6;
        lowerBoxA = lowerBox6L;
        lowerAreaA = lowerArea6L;
        lowerBoxB = lowerBox6R;
        lowerAreaB = lowerArea6R;
      }
    }
  }
  // Step 2: Apply that rotation and update all affected costs.
  nodes[ nodeID ].cost = bestCost / nodeArea;
  switch( best )
  {
    case 0:
      return;
    case 1:
      nodes[ leftID ].setBounds(lowerBoxA);
      nodes[ leftID ].cost = BVH_C_trav + ( ( leftRightArea * nodes[ leftRightID ].cost +
                                         rightArea * nodes[ rightID ].cost ) / lowerAreaA );
      swapNodes(leftLeftID, rightID);
      break;
    case 2:
      nodes[ leftID ].setBounds(lowerBoxA);
      nodes[ leftID ].cost = BVH_C_trav + ( ( leftLeftArea * nodes[ leftLeftID ].cost +
                                         rightArea * nodes[ rightID ].cost ) / lowerAreaA );
      swapNodes(leftRightID,rightID);
      break;
    case 3:
      nodes[ rightID ].setBounds(lowerBoxA);
      nodes[ rightID ].cost = BVH_C_trav + ( ( rightRightArea * nodes[ rightRightID ].cost +
                                          leftArea * nodes[ leftID ].cost ) / lowerAreaA );
      swapNodes(rightLeftID, leftID);
      break;
    case 4:
      nodes[ rightID ].setBounds(lowerBoxA);
      nodes[ rightID ].cost = BVH_C_trav + ( ( rightLeftArea * nodes[ rightLeftID ].cost +
                                          leftArea * nodes[ leftID ].cost ) / lowerAreaA );
      swapNodes(rightRightID,leftID);
      break;
    case 5:
      nodes[ leftID ].setBounds(lowerBoxA);
      nodes[ rightID ].setBounds(lowerBoxB);
      nodes[ leftID ].cost = BVH_C_trav + ( ( leftLeftArea * nodes[ leftLeftID ].cost +
                                         rightLeftArea * nodes[ rightLeftID ].cost ) / lowerAreaA );
      nodes[ rightID ].cost = BVH_C_trav + ( ( leftRightArea * nodes[ leftRightID ].cost +
                                          rightRightArea * nodes[ rightRightID ].cost ) / lowerAreaB );
      swapNodes(leftRightID,rightLeftID);
      break;
    case 6:
      nodes[ leftID ].setBounds(lowerBoxA);
      nodes[ rightID ].setBounds(lowerBoxB);
      nodes[ leftID ].cost = BVH_C_trav + ( ( leftLeftArea * nodes[ leftLeftID ].cost +
                                         rightRightArea * nodes[ rightRightID ].cost ) / lowerAreaA );
      nodes[ rightID ].cost = BVH_C_trav + ( ( leftRightArea * nodes[ leftRightID ].cost +
                                          rightLeftArea * nodes[ rightLeftID ].cost ) / lowerAreaB );
      swapNodes(leftRightID,rightRightID);
  }
}

void BVH::swapNodes(int i, int j)
{
  BVHNode temp = nodes[i];
  nodes[i] = nodes[j];
  nodes[j] = temp;
}

void BVH::rotateTree(int id)
{
  if(!nodes[id].isLeaf())
    {
      rotateTree(nodes[id].start_child);
      rotateTree(nodes[id].start_child + 1);
      rotateNode(id);
    }
}
#endif
