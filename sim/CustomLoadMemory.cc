#include "CustomLoadMemory.h"

void CustomLoadMemory(FourByte* mem, int size, int image_width, int image_height,
		      float epsilon, int custom_id) {

  switch(custom_id) {
  case 1:
    // run PIM loader
    break;
  default:
    // bare bones loader
    mem[0].uvalue = 0;
    mem[1].uvalue = image_width;
    mem[2].fvalue = 1.f/static_cast<float>(image_width);
    mem[3].fvalue = static_cast<float>(image_width);
    mem[4].uvalue = image_height;
    mem[5].fvalue = 1.f/static_cast<float>(image_height);
    mem[6].fvalue = static_cast<float>(image_height);
    mem[7].ivalue = 64;
    mem[14].uvalue = 64+image_width * image_height * 3; // start of free memory
    mem[15].uvalue = size-1; // end of free memory
    mem[18].fvalue = epsilon;
    break;
  }
}
