#include "OBJLoader.h"
#include "MTLLoader.h"
#include "Triangle.h"
#include "PPM.h"
#include <stdio.h>
#include <stdlib.h>
#include "Vector3.h"
#include <string.h>
#include <limits.h>

Vector3 GetPosition(int obj_pos_reference, std::vector<Vector3>& positions) {
  if (obj_pos_reference < 0) {
    return positions.at(positions.size() + obj_pos_reference);
  } else {
    return positions.at(obj_pos_reference - 1); // obj is 1 based indexing
  }
}

Vector3 GetTexPosition(int tex_pos_reference, std::vector<Vector3>& tex_coords) {
  if (tex_pos_reference == INT_MAX)
    return Vector3();
  if (tex_pos_reference < 0) {
    return tex_coords.at(tex_coords.size() + tex_pos_reference);
  } else {
    return tex_coords.at(tex_pos_reference - 1); // obj is 1 based indexing
  }
}

void OBJLoader::LoadModel(const char* filename, std::vector<Triangle*>* tris,
			  std::vector<Material*>* matls, FourByte* mem, int max_mem,
			  int& mem_loc, int matl_offset) {
  //printf("in load model, offset = %d\n", matl_offset);
  FILE* input = fopen(filename, "r");
  printf("loading model %s\n", filename);
  if (!input) {
    perror("Failed to open obj for reading.\n");
    exit(1);
  }


  std::vector<Vector3> texture_coords;
  std::vector<Vector3> positions;
  std::vector<std::string> matl_ids;
  int matl_id = 0;

#define BIG_VAL 1000000.0f
  static float max_x=-BIG_VAL, max_y=-BIG_VAL, max_z=-BIG_VAL, min_x=BIG_VAL, min_y=BIG_VAL, min_z=BIG_VAL;

  while (!feof(input)) {
    char line_buf[1024];
    char matl_filename[256];
    char matl_buf[128];
    fgets(line_buf, sizeof(line_buf), input);
    float x, y, z;

    // ignore comments and group commands
    if(line_buf[0] == '#' || line_buf[0] == 'g')
      continue;

    // read a vertex
    if (sscanf(line_buf, " v %f %f %f", &x, &y, &z) == 3) {
      
      positions.push_back(Vector3(x, y, z));

      // keep track of the bounds of the scene
      if (x > max_x) max_x = x;
      if (y > max_y) max_y = y;
      if (z > max_z) max_z = z;
      if (x < min_x) min_x = x;
      if (y < min_y) min_y = y;
      if (z < min_z) min_z = z;
    }
    // read a texture coordinate
    else if (sscanf(line_buf, " vt %f %f %f", &x, &y, &z) == 3 ||
	     sscanf(line_buf, " vt %f %f", &x, &y) == 2) {

      /*
      // handle wrapping texture coords (< -1, > 1)
      if(x != (int)x)
	x -= (int)x;
      if(y != (int)y)
	y -= (int)y;
      if(z != (int)z)
	z -= (int)z;
      
      // take care of negative texture coordinates
      if(x < 0)
	x = 1 - -x;
      if(y < 0)
	y = 1 - -y;
      if(z < 0)
	z = 1 - -z;
      */

      texture_coords.push_back(Vector3(x, y, z));
    }
    // read a mtllib command (points to a .mtl file)
    // bunch of messy code for handling this below -------------------
    else if (sscanf(line_buf, "mtllib %s", matl_buf) == 1) {
      // Determine mtl filename
      char * tmp = NULL, *tmp2 = NULL;      
      if(matl_buf[0]=='.'){ // relative path
	char *matl_buf_2 = matl_buf + 2;
	strncpy(matl_filename, filename, 256); 
	tmp2 = strstr(matl_filename, "/");
	while (tmp2 != NULL) {
	  tmp = tmp2++;
	  tmp2 = strstr(tmp2, "/");
	}
	strncpy(tmp++, "/", 1);
	strncpy(tmp, matl_buf_2, strlen(matl_buf_2));
	tmp[strlen(matl_buf_2)] = '\0';      
      }
      else if (matl_buf[0] == '/') { // absolute path
	strncpy(matl_filename, matl_buf, strlen(matl_buf));
	matl_filename[strlen(matl_filename)] = '\0';
      } else { // just filename
	strncpy(matl_filename, filename, strlen(filename)); 
	tmp2 = strstr(matl_filename, "/");
	while (tmp2 != NULL) {
	  tmp = tmp2++;
	  tmp2 = strstr(tmp2, "/");
	}
	strncpy(tmp++, "/", 1);
	strncpy(tmp, matl_buf, strlen(matl_buf));
	tmp[strlen(matl_buf)] = '\0';      
      }
      
      //printf("MTL file: \"%s\"\n", matl_filename);
      // read MTL file and store matl_ids as well as matls
      MTLLoader::LoadMTL(matl_filename, matls, matl_ids, mem, max_mem, mem_loc);
      //for(size_t i = 0; i < matls->size(); i++)
      //printf("matl[%d] tex pointers = %p %p %p\n", matls[i]->Ka_tex->name.c_str())
      // end mtl file code ------------------------
      

      // usemtl command
    } else if (line_buf[0] == 'u') {

      if (sscanf(line_buf, "usemtl %s", matl_buf) == 1) {

	unsigned int id = 0;
	while(id < matl_ids.size()) {
	  if (strcmp(matl_ids[id].c_str(), matl_buf) == 0) {
	    break;
	  }
	  id++;
	}
	//printf("matlid = %d, offset = %d\n", id, matl_offset);
	matl_id = id + matl_offset;
	//printf("after offset, matlid = %d\n", matl_id);
      }
      
      

      // face command (construct a quad or a triangle)
    } else if (line_buf[0] == 'f') {
      // found a face command
      int vert_ids[4] = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
      int tex_ids[4]  = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
      // quad vertex/texture/normal IDs
      if (sscanf(line_buf, "f %d/%d/%*d %d/%d/%*d %d/%d/%*d %d/%d/%*d",
		 &vert_ids[0], &tex_ids[0], &vert_ids[1], &tex_ids[1],
		 &vert_ids[2], &tex_ids[2], &vert_ids[3], &tex_ids[3]) == 8 ||
	  // quad vertex//normal IDs
	  sscanf(line_buf, "f %d//%*d %d//%*d %d//%*d %d//%*d",
		 &vert_ids[0], &vert_ids[1], &vert_ids[2], &vert_ids[3]) == 4 ||
	  // quad vertex/texture IDs
	  sscanf(line_buf, "f %d/%d %d/%d %d/%d %d/%d",
		 &vert_ids[0], &tex_ids[0], &vert_ids[1], &tex_ids[1], 
		 &vert_ids[2], &tex_ids[2], &vert_ids[3], &tex_ids[3]) == 8 ||
	  // quad vertex IDs only
	  sscanf(line_buf, "f %d %d %d %d",
		 &vert_ids[0], &vert_ids[1], &vert_ids[2], &vert_ids[3]) == 4 ||
	  // tri vertex/texture/normal IDs
	  sscanf(line_buf, "f %d/%d/%*d %d/%d/%*d %d/%d/%*d",
		 &vert_ids[0], &tex_ids[0], &vert_ids[1], &tex_ids[1], 
		 &vert_ids[2], &tex_ids[2]) == 6 ||
	  // tri vertex//normal IDs
	  sscanf(line_buf, "f %d//%*d %d//%*d %d//%*d",
		 &vert_ids[0], &vert_ids[1], &vert_ids[2]) == 3 ||
	  // tri vertex/texture IDs
	  sscanf(line_buf, "f %d/%d %d/%d %d/%d",
		 &vert_ids[0], &tex_ids[0], &vert_ids[1], &tex_ids[1], 
		 &vert_ids[2], &tex_ids[2]) == 6 ||
	  // tri vertex IDs only
	  sscanf(line_buf, "f %d %d %d",
		 &vert_ids[0], &vert_ids[1], &vert_ids[2]) == 3) {
	
	
	if (vert_ids[3] == INT_MAX) {
	  // one triangle
	  tris->push_back(new Triangle(GetPosition(vert_ids[0], positions),
				       GetPosition(vert_ids[1], positions),
				       GetPosition(vert_ids[2], positions),
				       GetTexPosition(tex_ids[0], texture_coords),
				       GetTexPosition(tex_ids[1], texture_coords),
				       GetTexPosition(tex_ids[2], texture_coords),
				       int(tris->size()), matl_id));
	} else {
	  // one quad
	  tris->push_back(new Triangle(GetPosition(vert_ids[0], positions),
				       GetPosition(vert_ids[1], positions),
				       GetPosition(vert_ids[2], positions),
				       GetTexPosition(tex_ids[0], texture_coords),
				       GetTexPosition(tex_ids[1], texture_coords),
				       GetTexPosition(tex_ids[2], texture_coords),
				       int(tris->size()), matl_id));
	  tris->push_back(new Triangle(GetPosition(vert_ids[0], positions),
				       GetPosition(vert_ids[2], positions),
				       GetPosition(vert_ids[3], positions),
				       GetTexPosition(tex_ids[0], texture_coords),
				       GetTexPosition(tex_ids[2], texture_coords),
				       GetTexPosition(tex_ids[3], texture_coords),
				       int(tris->size()), matl_id));
	}
	if(tris->at(tris->size()-1) == NULL)
	  {
	    printf("Error: malloc failed in OBJLoader\n");
	    exit(1);
	  }	
      }
    }
  }
  
  printf("%d total triangles\n", int(tris->size()));
  printf("vertex min/max = x: (%f, %f) y: (%f, %f) z: (%f, %f)\n", min_x, max_x, min_y, max_y, min_z, max_z);
  fclose(input);
}
