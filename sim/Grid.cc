#include "Grid.h"

#include <cstdlib>

using namespace simtrax;
double epsilon = 0.0001;

Grid::Grid(std::vector<Triangle*>* _triangles,  bool tris_store_edges, int dim)
{
  triangles_store_points = !tris_store_edges;
  triangles = _triangles;
  if(dim<=0)
  {
    printf("Grid: Error - grid dimension must be > 0\n");
    exit(1);
    // TODO: auto-calculate the dimensions (method in thiago's thesis)
  }
  dimensions = dim;
  cells = new std::vector<int>[dimensions*dimensions*dimensions];
  // cells = (std::vector<Triangle*>*)malloc(sizeof(std::vector<Triangle*>)*dimensions*dimensions*dimensions);
  if(!cells)
    {
      printf("Grid: Error - malloc failed\n");
      exit(1);
    }
  printf("preprocessing grid...\n");
  Preprocess();
  printf("done\n");
}

void Grid::LoadIntoMemory(int &memory_position,
			  int max_memory,
			  FourByte* memory)
{
  int i;
  size_t j;
  // load bounds, diagonal, cellsize
  for(i=0; i<3; i++)
    {
      memory[memory_position+i].fvalue   = static_cast<float>( bounds.box_min[i] );
      memory[memory_position+i+3].fvalue = static_cast<float>( bounds.box_max[i] );
      memory[memory_position+i+6].fvalue = static_cast<float>( diagonal[i] );
      memory[memory_position+i+9].fvalue = static_cast<float>( cellSize[i] );
    }
  memory_position+=12;

  // load dimensions
  memory[memory_position].ivalue = dimensions;
  memory_position++;
  int start_cells = memory_position;
  memory_position++;
  // load the triangles
  int start_triangles = memory_position;
  for(j=0; j<triangles->size(); j++)
    triangles->at(j)->LoadIntoMemory(memory_position, max_memory, memory);

  // pointer to begining of grid cells
  memory[start_cells].ivalue = memory_position;
  // load grid cells (triangle count and pointers to triangles)
  int grid_base_address = memory_position;
  memory_position += dimensions*dimensions*dimensions*2;
  for(i=0; i<dimensions*dimensions*dimensions; i++)
    {
      memory[grid_base_address].ivalue = cells[i].size();
      memory[grid_base_address+1].ivalue = memory_position;

      // load triangle pointers for this grid cell
      for(j=0; j<cells[i].size(); j++)
	{
	  memory[memory_position].ivalue = start_triangles + (cells[i].at(j) * 11);
	  memory_position++;
	}
      grid_base_address+=2;
    }
}

void Grid::Preprocess()
{
  if(triangles->size()==0)
    return;
  double minx, miny, minz, maxx, maxy, maxz, xrange, yrange, zrange;
  int i;

  // compute bounding volume for entire grid
  bounds = GetTriangleBoundingBox(*triangles->at(0), triangles_store_points);
  for(i=0; i<(int)(triangles->size()); i++)
    ExtendBoundsByTriangle(*triangles->at(i), triangles_store_points);      
  
  bounds.box_max+=Vector3(epsilon, epsilon, epsilon);
  bounds.box_min-=Vector3(epsilon, epsilon, epsilon);

  diagonal = bounds.box_max-bounds.box_min;
  cellSize = diagonal/(double)dimensions;

  minx = bounds.box_min.x();
  miny = bounds.box_min.y();
  minz = bounds.box_min.z();
  maxx = bounds.box_max.x();
  maxy = bounds.box_max.y();
  maxz = bounds.box_max.z();
  xrange = maxx-minx;
  yrange = maxy-miny;
  zrange = maxz-minz;

  
  

  for(i=0; i<(int)(triangles->size()); i++)
    {
	  Box b = GetTriangleBoundingBox(*triangles->at(i), triangles_store_points);
      int Lx, Hx, Ly, Hy, Lz, Hz;
      Lx = (int)floor((((b.box_min.x()-minx)/xrange))*double(dimensions));
      Ly = (int)floor((((b.box_min.y()-miny)/yrange))*double(dimensions));
      Lz = (int)floor((((b.box_min.z()-minz)/zrange))*double(dimensions));

      Hx = (int)floor((((b.box_max.x()-minx)/xrange))*double(dimensions));
      Hy = (int)floor((((b.box_max.y()-miny)/yrange))*double(dimensions));
      Hz = (int)floor((((b.box_max.z()-minz)/zrange))*double(dimensions));
      
      if(Lx==dimensions)
	Lx--;
      if(Ly==dimensions)
	Ly--;
      if(Lz==dimensions)
	Lz--;
      if(Hx==dimensions)
	Hx--;
      if(Hy==dimensions)
	Hy--;
      if(Hz==dimensions)
	Hz--;

      int j, k, l;
      for(j=Lx; j<=Hx; j++)
	for(k=Ly; k<=Hy; k++)
	  for(l=Lz; l<=Hz; l++)
	    {
	      fflush(stdout);
	      AddTriangleToCell(i, j, k, l);
	      fflush(stdout);
	    }
    }
}

void Grid::ExtendBoundsByTriangle(const Triangle& tri, const bool &tri_store_pts) 
{
	if( tri_store_pts ) {
	  for (int i = 0; i < 3; i++) {
		if (tri.p0[i] < bounds.box_min[i]) {
		  bounds.box_min[i] = tri.p0[i] - epsilon;
		}
		if (tri.p1[i] < bounds.box_min[i]) {
		  bounds.box_min[i] = tri.p1[i] - epsilon;
		}
		if (tri.p2[i] < bounds.box_min[i]) {
		  bounds.box_min[i] = tri.p2[i] - epsilon;
		}

		if (tri.p0[i] > bounds.box_max[i]) {
		  bounds.box_max[i] = tri.p0[i] + epsilon;
		}
		if (tri.p1[i] > bounds.box_max[i]) {
		  bounds.box_max[i] = tri.p1[i] + epsilon;
		}
		if (tri.p2[i] > bounds.box_max[i]) {
		  bounds.box_max[i] = tri.p2[i] + epsilon;
		}
	  }
	}
	else {
		for (int i = 0; i < 3; i++) {
			const double tmp0 = tri.p0[i] + tri.p1[i];
			if( tmp0 < bounds.box_min[i] ) {
				bounds.box_min[i] = tmp0 - epsilon;
			}
			if( tmp0 > bounds.box_max[i] ) {
				bounds.box_max[i] = tmp0 + epsilon;
			}

			if( tri.p1[i] < bounds.box_min[i] ) {
				bounds.box_min[i] = tri.p1[i] - epsilon;
			}
			if( tri.p1[i] > bounds.box_max[i] ) {
				bounds.box_max[i] = tri.p1[i] + epsilon;
			}

			const double tmp2 = tri.p2[i] + tri.p1[i];
			if( tmp2 < bounds.box_min[i] ) {
				bounds.box_min[i] = tmp2 - epsilon;
			}
			if( tmp2 > bounds.box_max[i] ) {
				bounds.box_max[i] = tmp2 + epsilon;
			}
		}
	}
}

Box Grid::GetTriangleBoundingBox(const Triangle& tri, const bool &tri_store_pts)
{
  double p0x = tri.p0.x(), p0y = tri.p0.y(), p0z = tri.p0.z();
  double p2x = tri.p2.x(), p2y = tri.p2.y(), p2z = tri.p2.z();
  if( tri_store_pts==false ) {
	  p0x += tri.p1.x();
	  p0y += tri.p1.y();
	  p0z += tri.p1.z();
	  p2x += tri.p1.x();
	  p2y += tri.p1.y();
	  p2z += tri.p1.z();
  }
  Box retval;
  retval.box_min.setX(min(min(p0x, tri.p1.x()), p2x) - epsilon);
  retval.box_min.setY(min(min(p0y, tri.p1.y()), p2y) - epsilon);
  retval.box_min.setZ(min(min(p0z, tri.p1.z()), p2z) - epsilon);
  retval.box_max.setX(max(max(p0x, tri.p1.x()), p2x) + epsilon);
  retval.box_max.setY(max(max(p0y, tri.p1.y()), p2y) + epsilon);
  retval.box_max.setZ(max(max(p0z, tri.p1.z()), p2z) + epsilon);
  return retval;
}

void Grid::AddTriangleToCell(int addr, int x, int y, int z)
{
  cells[cellindex(x, y, z)].push_back(addr);
}
