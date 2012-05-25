#include "trax.hpp"
#include "Image.h"

#define MAX_ITERS 100
void trax_main()
{
  float zreal;
  float zimag;
  float creal;
  float cimag;
  float lengthsq;
  float temp;
  int start_fb = loadi(7);
  int xres = loadi(1);
  int yres = loadi(4);
  int total_pixels = xres * yres;
  int i;
  int j;
  int k;
  
  // set up frame buffer
  Image image(xres, yres, start_fb);

  for(int pix = atomicinc(0); pix < xres * yres; pix = atomicinc(0))
    {
      i = pix / xres;
      j = pix % xres;
      zreal = 0.0;
      zimag = 0.0;
      creal = (j - xres/1.4) / (xres/2.0);
      cimag = (i - yres/2.0) / (yres/2.0);
      k = 0;
      do
	{
	  temp = (zreal * zreal) - (zimag * zimag) + creal;
	  zimag = (2.0 * zreal * zimag) + cimag;
	  zreal = temp;
	  lengthsq = (zreal * zreal) + (zimag * zimag);
	  ++k;
	}
      while(lengthsq < 4.0 && k < MAX_ITERS);
      if(k==MAX_ITERS)
	k = 0;
      float intensity = k / (float)MAX_ITERS;
      Color color(intensity, 0.f, 0.f); // simple red shading
      image.set(i, j, color);
    }
}
