#include "IWLoader.h"
#include "Triangle.h"
#include "Material.h"
#include <stdio.h>
#include <stdlib.h>
#include "Vector3.h"
#include <iostream>
#include <string.h>

using namespace simtrax;

void IWLoader::LoadModel(const char* filename, std::vector<Triangle*>* tris,
             std::vector<Material*>* matls) {
  FILE* input = fopen(filename, "r");
  if (!input) {
    perror("Failed to open obj file for reading.\n");
    return;
  }

  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
  std::vector<Vector3> diffuse_colors;
  char buf[1024];

  while (fgets(buf, sizeof(buf), input) != NULL) {

    if (strstr(buf, "vertices") != 0) {
      // fill in mesh member variable
      int num_vertices = 0;
      sscanf(buf, "vertices: %d", &num_vertices);

      float values[3];
      for ( int i = 0; i < num_vertices; i++ ) {
        fgets(buf, sizeof(buf), input);

        sscanf(buf, "%f %f %f",
               &values[0],
               &values[1],
               &values[2]);

        positions.push_back(Vector3(values[0], values[1], values[2]));
      }
    }
    else if (strstr(buf, "vtxnormals") != 0) {
      int num_normals = 0;
      sscanf(buf, "vtxnormals: %d", &num_normals);
      if ( num_normals > 0 ) {
          if ( (unsigned int)num_normals != positions.size() ) {
            std::cerr << "Number of normals ("<<num_normals<<") doesn't match"
                      << " vertex count ("<<positions.size()<<")\n";
          }

          float values[3];
          for ( int i = 0; i < num_normals; i++ ) {
            fgets(buf, sizeof(buf), input);
            sscanf(buf, "%f %f %f",
                   &values[0],
                   &values[1],
                   &values[2]);

            normals.push_back(Vector3(values[0], values[1], values[2]));
          }
      }
    }
    else if ( strstr(buf, "triangles") != 0 ) {
      int num_triangles = 0;
      sscanf(buf, "triangles: %d", &num_triangles);

      int data[4];
      for ( int i = 0; i < num_triangles; i++ ) {
        fgets(buf, sizeof(buf), input);
        sscanf(buf, "%d %d %d %d",
               &data[0], &data[1], &data[2], &data[3]);

        tris->push_back(new Triangle(positions[data[0]],
                                    positions[data[1]],
                                    positions[data[2]],
                                    int(tris->size()), data[3]));
      }
    } else if (strstr(buf, "shaders") != 0 ) {
      int num_shaders = 0;
      sscanf(buf, "shaders %d", &num_shaders);

      int shader_id;
      float data[3];

      diffuse_colors.resize(num_shaders, Vector3(0.8, 0.8, 0.8));
      matls->resize(num_shaders, new Material(diffuse_colors[0],
					     diffuse_colors[0],
					     diffuse_colors[0]));
      for ( int i = 0; i < num_shaders; i++ ) {
        fgets(buf, sizeof(buf), input);
        sscanf(buf, "shader %d diffuse (%f,%f,%f)",
               &shader_id,
               &data[0],&data[1],&data[2]);

        diffuse_colors[shader_id] = Vector3(data[0], data[1], data[2]);
	matls->at(shader_id) = new Material(diffuse_colors[shader_id], 
					diffuse_colors[shader_id], 
					diffuse_colors[shader_id],
					2); // 2 shader type is diffuse
	//TODO: Make this handle other shader types, not just "diffuse"
      }
    }
  }

  printf("Found %d total triangles\n", int(tris->size()));

  fclose(input);
}
