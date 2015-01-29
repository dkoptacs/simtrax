#include "OBJListLoader.h"

using namespace simtrax;

void OBJListLoader::LoadModel(const char* filename, std::vector<Triangle*>* tris,
			  std::vector<Material*>* matls, FourByte* mem, int max_mem,
			  int& mem_loc) {
  FILE* input = fopen(filename, "r");
  printf("loading obj list %s\n", filename);
  if (!input) 
    {
      perror("Failed to open obj list file for reading.\n");
      exit(1);
    }

  // Keep track of how many materials each obj file references
  int matl_offset = 0;

  while (!feof(input)) 
    {
      char line_buf[1024];
      char obj_filename[1024];
      fgets(line_buf, sizeof(line_buf), input);
      if(line_buf[strlen(line_buf)-1] == '\n')
	line_buf[strlen(line_buf)-1] = '\0';
      
      if(strlen(line_buf) == 0)
	continue;
      if(line_buf[0] == '\n')
	continue;

      // Determine mtl filename
      char * tmp = NULL, *tmp2 = NULL;      
      if(line_buf[0]=='.'){ // relative path
	char *matl_buf_2 = line_buf + 2;
	strncpy(obj_filename, filename, 256); 
	tmp2 = strstr(obj_filename, "/");
	while (tmp2 != NULL) {
	  tmp = tmp2++;
	  tmp2 = strstr(tmp2, "/");
	}
	strncpy(tmp++, "/", 1);
	strncpy(tmp, matl_buf_2, strlen(matl_buf_2));
	tmp[strlen(matl_buf_2)] = '\0';      
      }
      else if (line_buf[0] == '/') { // absolute path
	strncpy(obj_filename, line_buf, strlen(line_buf));
	obj_filename[strlen(obj_filename)] = '\0';
      } else { // just filename
	strncpy(obj_filename, filename, strlen(filename)); 
	tmp2 = strstr(obj_filename, "/");
	while (tmp2 != NULL) {
	  tmp = tmp2++;
	  tmp2 = strstr(tmp2, "/");
	}
	strncpy(tmp++, "/", 1);
	strncpy(tmp, line_buf, strlen(line_buf));
	tmp[strlen(line_buf)] = '\0';      
      }
      //obj_filename[strlen(obj_filename)] = '\0';
      //printf("matlssize = %d\n", matls->size());
      OBJLoader::LoadModel(obj_filename, tris, matls, mem, max_mem, mem_loc, matl_offset);
      matl_offset = matls->size();
    }
  
  fclose(input);
}
