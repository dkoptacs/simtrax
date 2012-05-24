#include "Triangle.h"

// by default we're storing points
// if flag enabled, p1 is point, but p0 = p0-p1, p2 = p2-p1 are edge vectors
bool Triangle::tri_stores_edges = false;


Triangle::Triangle(const Vector3& p0, const Vector3& p1, const Vector3& p2,
		   const Vector3& t0, const Vector3& t1, const Vector3& t2,
                   int object_id, int shader_id) :
  p0(p0), p1(p1), p2(p2), t0(t0), t1(t1), t2(t2), object_id(object_id), shader_id(shader_id)
{
	// do we compute 2 edges to store instead of points?
	// p0 (edge between p1 and p0), p1 (central point), p2 (edge between p2 and p1)
	if( Triangle::tri_stores_edges ) {
		this->p0 -= p1;
		this->p2 -= p1;
	}
}

Triangle::Triangle(const Vector3& p0, const Vector3& p1, const Vector3& p2,
                   int object_id, int shader_id) :
  p0(p0), p1(p1), p2(p2), object_id(object_id), shader_id(shader_id)
{
	// do we compute 2 edges to store instead of points?
	// p0 (edge between p1 and p0), p1 (central point), p2 (edge between p2 and p1)
	if( Triangle::tri_stores_edges ) {
		this->p0 -= p1;
		this->p2 -= p1;
	}  
}

Triangle::Triangle(const Triangle& t) :
p0(t.p0), p1(t.p1), p2(t.p2), object_id(t.object_id), shader_id(t.shader_id)
{
  
}

Triangle::Triangle()
{}

void Triangle::LoadIntoMemory(int& memory_position, int max_memory,
                              FourByte* memory) {
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( p0[i] );
    memory_position++;
  }
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( p1[i] );
    memory_position++;
  }
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( p2[i] );
    memory_position++;
  }
  memory[memory_position].ivalue = object_id;
  memory_position++;
  memory[memory_position].ivalue = shader_id;
  memory_position++;
}

// Load these separately, so they don't have to be loaded if not desired
void Triangle::LoadTextureCoords(int& memory_position, int max_memory,
				 FourByte* memory)
{
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( t0[i] );
    memory_position++;
  }
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( t1[i] );
    memory_position++;
  }
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( t2[i] );
    memory_position++;
  }  
}
