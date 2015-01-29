#ifndef _SIMHWRT_GRID_H_
#define _SIMHWRT_GRID_H_

#include "FourByte.h"
#include "Primitive.h"
#include "Triangle.h"
#include <vector>

namespace simtrax {
  class Triangle;
}

struct Box
{
  Vector3 box_min;
  Vector3 box_max;
};

class Grid : public simtrax::Primitive {
  int dimensions;
  std::vector<int> *cells;
  std::vector<simtrax::Triangle*>* triangles;
  Box bounds;
  Vector3 diagonal;
  Vector3 cellSize;
  void Preprocess();

  // tri_store_pts = true  -> triangles are normal (3 points for vertices)
  // tri_store_pts = false -> triangles storing edge vectors (center pt + vectors to other 2)
  void ExtendBoundsByTriangle(const simtrax::Triangle& tri, const bool &tri_store_pts=true);
  Box GetTriangleBoundingBox(const simtrax::Triangle& tri, const bool &tri_store_pts=true);
  void AddTriangleToCell(int addr, int x, int y, int z);

  int cellindex(int x, int y, int z)
  {
    return ((z*dimensions)+y)*dimensions + x;
  }
  bool triangles_store_points;


 public:
  Grid(std::vector<simtrax::Triangle*>* _triangles, bool tris_store_edges=false, int dim=0); // if no dim passed, auto calculate
  void LoadIntoMemory(int &memory_position, int max_memory, FourByte* memory);

  
};

inline double max(double a, double b)
{
  return a>b ? a : b;
}
inline double min(double a, double b)
{
  return a<b ? a : b;
}


#endif
