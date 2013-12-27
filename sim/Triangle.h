#ifndef _SIMHWRT_TRIANGLE_H_
#define _SIMHWRT_TRIANGLE_H_

#include "Primitive.h"
#include "Vector3.h"

class Triangle : public Primitive {
public:
  Triangle();
  // Verts, texture coords, and normals
  Triangle(const Vector3& p0, const Vector3& p1, const Vector3& p2,
	   const Vector3& t0, const Vector3& t1, const Vector3& t2,
	   const Vector3& n0, const Vector3& n1, const Vector3& n2,
           int object_id, int shader_id);
  // Verts and texture coords
  Triangle(const Vector3& p0, const Vector3& p1, const Vector3& p2,
	   const Vector3& t0, const Vector3& t1, const Vector3& t2,
           int object_id, int shader_id);
  // Just verts
  Triangle(const Vector3& p0, const Vector3& p1, const Vector3& p2,
           int object_id, int shader_id);
  Triangle(const Triangle& t);

  virtual void LoadIntoMemory(int& memory_position, int max_memory,
                              FourByte* memory);
  void LoadTextureCoords(int& memory_position, int max_memory,
				 FourByte* memory);

  void LoadVertexNormals(int& memory_position, int max_memory,
			 FourByte* memory);
  
  Vector3 p0; // vertices
  Vector3 p1;
  Vector3 p2;
  Vector3 t0; // texture coords
  Vector3 t1;
  Vector3 t2;
  Vector3 n0; // vertex normals
  Vector3 n1;
  Vector3 n2;
  int object_id;
  int shader_id;

  static bool tri_stores_edges;	/// flag whether we're storing edge vectors instead of points: p1-center, p0=p0-p1, p2=p2-p1
};

#endif // _SIMHWRT_TRIANGLE_H_
