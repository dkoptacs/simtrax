#include "MTLLoader.h"
#include "Material.h"
#include "PPM.h"
#include <stdio.h>
#include <stdlib.h>
#include "Vector3.h"
#include "FourByte.h"
#include <string.h>
#include <string>
#include "TGALoader.h"

void MTLLoader::LoadMTL(const char* filename, std::vector<Material*>* matls,
			std::vector<std::string>& matl_ids, FourByte* mem, int max_mem,
			int& mem_loc) {
  FILE* input = fopen(filename, "r");
  if (!input) {
    perror("Failed to open mtl file for reading.\n");
    return;
  }
  printf("loading material file %s\n", filename);
  Material* new_matl = NULL;
  while (!feof(input)) {
    char line_store[1024];
    char color_buf[256];
    char tex_buf[128];
    fgets(line_store, sizeof(line_store), input);
    char *line_buf = line_store;

    // trim leading whitespace
    while(line_buf[0] == ' ' || line_buf[0] == '\t' || line_buf[0] == '\n')
      line_buf++; 
    if(strlen(line_buf) < 1)
      continue;

    float r, g, b;
    float indexOfRefraction;
    float s0, s1, s2, o0, o1, o2;

    if (line_buf[0] == '#') {
      continue;
    } else if (sscanf(line_buf, "newmtl %s", color_buf) == 1) {
      matl_ids.push_back(std::string(color_buf));
      //printf("adding mat: %s\n", color_buf);
      new_matl = new Material(Vector3(0,0,0), Vector3(0,0,0), Vector3(0,0,0));
      matls->push_back(new_matl);
    } else if (sscanf(line_buf, "illum %d", &new_matl->mat_type) == 1) {
      // no body
    } else if (sscanf(line_buf, "Ka  %f %f %f", &r, &g, &b) == 3) {
      new_matl->c0 = Vector3(r, g, b);
    } else if (sscanf(line_buf, "Kd  %f %f %f", &r, &g, &b) == 3) {
      new_matl->c1 = Vector3(r, g, b);
    } else if (sscanf(line_buf, "Ks  %f %f %f", &r, &g, &b) == 3) {
      new_matl->c2 = Vector3(r, g, b);
    } else if (sscanf(line_buf, "Ke  %f %f %f", &r, &g, &b) == 3) {
      //emission... skipped for now
      //new_matl->c3 = Vector3(r, g, b);

      // Index of refraction
    } else if (sscanf(line_buf, "Ni %f", &indexOfRefraction) == 1) {
      // Just use one of the texture scale compontents for now
      // TODO: Add an Ni term to materials, this will break all old ray tracers that assume materials are 25 words.
      //       We really need to define a SIZE for materials, triangles, boxes, etc... so we don't break old code when the size changes
      new_matl->sd0 = indexOfRefraction;
      
    }else if (sscanf(line_buf, "map_Ka -s %f %f %f -o %f %f %f %s", 
		      &s0, &s1, &s2, &o0, &o1, &o2, tex_buf) == 7) {

      if(new_matl->Kd_tex != NULL && new_matl->Kd_tex->name.compare(tex_buf) == 0)
	new_matl->Ka_tex = new_matl->Kd_tex;
      else if(new_matl->Ks_tex != NULL && new_matl->Ks_tex->name.compare(tex_buf) == 0)
	new_matl->Ka_tex = new_matl->Ks_tex;
      else
	new_matl->Ka_tex = ReadTexture(filename, tex_buf, matls);
      // TODO: push this texture pointer on to a vector, load them all after the materials have been loaded

      // set scales
      new_matl->sa0 = s0;
      new_matl->sa1 = s1;
      new_matl->oa0 = o0;
      new_matl->oa1 = o1;
    } else if (sscanf(line_buf, "map_Kd -s %f %f %f -o %f %f %f %s", 
		      &s0, &s1, &s2, &o0, &o1, &o2, tex_buf) == 7) {

      if(new_matl->Ka_tex != NULL && new_matl->Ka_tex->name.compare(tex_buf) == 0)
	new_matl->Kd_tex = new_matl->Ka_tex;
      else if(new_matl->Ks_tex != NULL && new_matl->Ks_tex->name.compare(tex_buf) == 0)
	new_matl->Kd_tex = new_matl->Ks_tex;
      else
	new_matl->Kd_tex = ReadTexture(filename, tex_buf, matls);
      // set scales
      new_matl->sd0 = s0;
      new_matl->sd1 = s1;
      new_matl->od0 = o0;
      new_matl->od1 = o1;
    } else if (sscanf(line_buf, "map_Ks -s %f %f %f -o %f %f %f %s", 
		      &s0, &s1, &s2, &o0, &o1, &o2, tex_buf) == 7) {
      
      if(new_matl->Ka_tex != NULL && new_matl->Ka_tex->name.compare(tex_buf) == 0)
	new_matl->Ks_tex = new_matl->Ka_tex;
      else if(new_matl->Kd_tex != NULL && new_matl->Kd_tex->name.compare(tex_buf) == 0)
	new_matl->Ks_tex = new_matl->Kd_tex;
      else
	new_matl->Ks_tex = ReadTexture(filename, tex_buf, matls);
      
      // set scales
      new_matl->ss0 = s0;
      new_matl->ss1 = s1;
      new_matl->os0 = o0;
      new_matl->os1 = o1;
    } else if (sscanf(line_buf, "map_Ks %s", tex_buf) == 1) {
      if(new_matl->Ka_tex != NULL && new_matl->Ka_tex->name.compare(tex_buf) == 0)
	new_matl->Ks_tex = new_matl->Ka_tex;
      else if(new_matl->Kd_tex != NULL && new_matl->Kd_tex->name.compare(tex_buf) == 0)
	new_matl->Ks_tex = new_matl->Kd_tex;
      else
	new_matl->Ks_tex = ReadTexture(filename, tex_buf, matls);
      // TODO: assign the ks pointer (i0) to this texture
      //TODO: Check if the ka or kd textures map to the same image,
      // if so, don't load them twice (do the analogue of this below as well)
    } else if (sscanf(line_buf, "map_Ka %s", tex_buf) == 1) {
      if(new_matl->Kd_tex != NULL && new_matl->Kd_tex->name.compare(tex_buf) == 0)
	new_matl->Ka_tex = new_matl->Kd_tex;
      else if(new_matl->Ks_tex != NULL && new_matl->Ks_tex->name.compare(tex_buf) == 0)
	new_matl->Ka_tex = new_matl->Ks_tex;
      else
	new_matl->Ka_tex = ReadTexture(filename, tex_buf, matls);
      // TODO: assign the ka pointer to this texture
    } else if (sscanf(line_buf, "map_Kd %s", tex_buf) == 1) {
      if(new_matl->Ka_tex != NULL && new_matl->Ka_tex->name.compare(tex_buf) == 0)
	new_matl->Kd_tex = new_matl->Ka_tex;
      else if(new_matl->Ks_tex != NULL && new_matl->Ks_tex->name.compare(tex_buf) == 0)
	new_matl->Kd_tex = new_matl->Ks_tex;
      else
	new_matl->Kd_tex = ReadTexture(filename, tex_buf, matls);
      // TODO: assign the kd pointer to this texture
    }
    
    // don't support Tr, Tf, d, bump, map_bump, map_d (dissolve) yet
    else if (line_buf[0] == 'd' || line_buf[0] == 'N' || line_buf[0] == 'T' || 
	     line_buf[0] == 'b' || line_buf[4] == 'b' || line_buf[4] == 'd') {
      // not used yet
    } else {
      printf("Bad line in mtl file: \"%s\"", line_buf);
      printf("using white\n");
      if(!new_matl){
	new_matl = new Material(Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1));
      }
      else{
	new_matl->c0 = Vector3(1, 1, 1);
	new_matl->c1 = Vector3(1, 1, 1);
	new_matl->c2 = Vector3(1, 1, 1);
      }
      //printf("made new mat\n");
      //fflush(stdout);
    }
  }

  printf("%d total materials\n", int(matls->size()));
  fclose(input);

}


// keep a vector of STGA*, so that they can be loaded after all of the other material data
//TODO: Check performance vs current repository version on a scene with no textures
STGA* MTLLoader::ReadTexture(const char* filename, char* tex_buf, std::vector<Material*>* matls)
{
  STGA *temp = new STGA();
  temp->name = tex_buf;
  for(size_t i = 0; i < matls->size(); i++)
    {
      if(matls->at(i)->Ka_tex != NULL && matls->at(i)->Ka_tex->name.compare(tex_buf) == 0)
	return matls->at(i)->Ka_tex;
      if(matls->at(i)->Kd_tex != NULL && matls->at(i)->Kd_tex->name.compare(tex_buf) == 0)
	return matls->at(i)->Kd_tex;
      if(matls->at(i)->Ks_tex != NULL && matls->at(i)->Ks_tex->name.compare(tex_buf) == 0)
	return matls->at(i)->Ks_tex;
    }

  // replace windows slashes with linux slashes
  char *p = tex_buf;
  while(*p != '\0')
    {
      if(*p == '\\')
	*p = '/';
      p++;
    }
  // Determine image filename with path
  char tex_filename[256];
  strncpy(tex_filename, filename, strlen(filename));
  char * tmp = NULL, *tmp2 = NULL;
  tmp2 = strstr(tex_filename, "/");
  while (tmp2 != NULL) {
    tmp = tmp2++;
    tmp2 = strstr(tmp2, "/");
  }
  strncpy(tmp++, "/", 1);
  strncpy(tmp, tex_buf, strlen(tex_buf));
  tmp[strlen(tex_buf)] = '\0';
  
  loadTGA(tex_filename, temp);
  
  return temp;
}
