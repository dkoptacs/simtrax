#ifndef _HWRT_CUSTOMLOADMEMORY_H_
#define _HWRT_CUSTOMLOADMEMORY_H_

#include "FourByte.h"

#include <vector>


void CustomLoadMemory(FourByte* mem, int size, int image_width, int image_height,
		      float epsilon, int custom_id);


#endif // _HWRT_CUSTOMLOADMEMORY_H_
