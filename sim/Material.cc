#include "Material.h"

using namespace simtrax;

Material::Material(const Vector3& c0, const Vector3& c1, const Vector3& c2, const int& mtl_type) :
  c0(c0), c1(c1), c2(c2)
{
  mat_type = mtl_type;
  Ka_tex = NULL;
  Kd_tex = NULL;
  Ks_tex = NULL;
}

void Material::LoadIntoMemory(int& memory_position, int max_memory,
                              FourByte* memory) { 
  memory[memory_position].ivalue = mat_type;
  memory_position++;
 for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( c0[i] );
    memory_position++;
  }
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( c1[i] );
    memory_position++;
  }
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( c2[i] );
    memory_position++;
  }
  if(Ka_tex == NULL)
    i0 = -1;
  else
    i0 = memory_position; // keep track of pointer location so 2nd pass can write the value
  memory[memory_position].ivalue = i0;
  memory_position++;
  // reserved for ou, ov, su, sv
  memory[memory_position].fvalue = sa0;
  memory_position++;
  memory[memory_position].fvalue = sa1;
  memory_position++;
  memory[memory_position].fvalue = oa0;
  memory_position++;
  memory[memory_position].fvalue = oa1;
  memory_position++;

  if(Kd_tex == NULL)
    i1 = -1;
  else
    i1 = memory_position; // keep track of pointer location so 2nd pass can write the value
  memory[memory_position].ivalue = i1;
  memory_position++;
  // reserved for ou, ov, su, sv
  memory[memory_position].fvalue = sd0;
  memory_position++;
  memory[memory_position].fvalue = sd1;
  memory_position++;
  memory[memory_position].fvalue = od0;
  memory_position++;
  memory[memory_position].fvalue = od1;
  memory_position++;

  if(Ks_tex == NULL)
    i2 = -1;
  else
    i2 = memory_position; // keep track of pointer location so 2nd pass can write the value
  memory[memory_position].ivalue = i2;
  memory_position++;
  // reserved for ou, ov, su, sv
  memory[memory_position].fvalue = ss0;
  memory_position++;
  memory[memory_position].fvalue = ss1;
  memory_position++;
  memory[memory_position].fvalue = os0;
  memory_position++;
  memory[memory_position].fvalue = os1;
  memory_position++;

}

void Material::LoadTextureIntoMemory(int& memory_position, int max_memory,
				     FourByte* memory) { 
  //TODO: There may be some more info I can store here to make interpolation faster (inv_width, etc)
  if(Ka_tex != NULL)
    {
      if(Ka_tex->address > 0) // if the teture was already loaded (pointed to by multiple mats)
	memory[i0].ivalue = Ka_tex->address;
      else
	{
	  //printf("uploading Ka: %s\n", Ka_tex->name.c_str());
	  Ka_tex->address = memory_position;
	  memory[i0].ivalue = memory_position;
	  memory[memory_position++].ivalue = Ka_tex->height;
	  memory[memory_position++].ivalue = Ka_tex->width;
	  memory[memory_position++].ivalue = Ka_tex->byteCount;
	  for(int i = 0; i < Ka_tex->height * Ka_tex->width; i++)
	    for(int j = Ka_tex->byteCount - 1; j >= 0; j--) // tga files are little endian
	      memory[memory_position++].ivalue = Ka_tex->data[(i * Ka_tex->byteCount) + j];
	}
    }
  if(Kd_tex != NULL)
    {
      if(Kd_tex == Ka_tex)
	{
	  memory[i1] = memory[i0];
	}
      else
	{
	  if(Kd_tex->address > 0) // if the teture was already loaded (pointed to by multiple mats)
	    memory[i1].ivalue = Kd_tex->address;
	  else
	    {
	      //printf("memory_position = %d\n", memory_position);
	      //printf("uploading Kd: %s\n", Kd_tex->name.c_str());
	      //printf("height = %d, width = %d\n", Kd_tex->height, Kd_tex->width);
	      //printf("i1 = %d\n", i1);
	      //fflush(stdout);
	      Kd_tex->address = memory_position;
	      memory[i1].ivalue = memory_position;
	      memory[memory_position++].ivalue = Kd_tex->height;
	      memory[memory_position++].ivalue = Kd_tex->width;
	      memory[memory_position++].ivalue = Kd_tex->byteCount;
	      for(int i = 0; i < Kd_tex->height * Kd_tex->width; i++)
		for(int j = Kd_tex->byteCount - 1; j >= 0; j--) // tga files are little endian
		  memory[memory_position++].ivalue = Kd_tex->data[(i * Kd_tex->byteCount) + j];
	    }
	}
      //printf("Kd address = %d\n", memory[i1].ivalue);
    }  
  if(Ks_tex != NULL)
    {
      if(Ks_tex == Ka_tex)
	memory[i2] = memory[i0];
      else if(Ks_tex == Kd_tex)
	memory[i2] = memory[i1];
      else
	{
	  if(Ks_tex->address > 0) // if the teture was already loaded (pointed to by multiple mats)
	    memory[i2].ivalue = Ks_tex->address;
	  else
	    {
	      //printf("uploading Ks: %s\n", Ks_tex->name.c_str());
	      Ks_tex->address = memory_position;
	      memory[i2].ivalue = memory_position;
	      memory[memory_position++].ivalue = Ks_tex->height;
	      memory[memory_position++].ivalue = Ks_tex->width;
	      memory[memory_position++].ivalue = Ks_tex->byteCount;
	      for(int i = 0; i < Ks_tex->height * Ks_tex->width; i++)
		for(int j = Ks_tex->byteCount - 1; j >= 0; j--) // tga files are little endian
		  memory[memory_position++].ivalue = Ks_tex->data[(i * Ks_tex->byteCount) + j];
	    }
	}
    }
}
