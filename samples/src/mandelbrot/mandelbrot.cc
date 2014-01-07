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
  int start_fb = GetFrameBuffer();
  int xres = GetXRes();
  int yres = GetYRes();
  int total_pixels = xres * yres;
  int i;
  int j;
  int k;
  
  // set up the frame buffer
  Image image(xres, yres, start_fb);

  // compute the Mandelbrot fractal
  // use atomic increment to loop through pixels so each thread gets unique assignments
  for(int pix = atomicinc(0); pix < xres * yres; pix = atomicinc(0))
    {
      // convert 1-d loop in to 2-d loop indices
      i = pix / xres;
      j = pix % xres;

      zreal = 0.0f;
      zimag = 0.0f;
      creal = (j - xres/1.4f) / (xres/2.0f);
      cimag = (i - yres/2.0f) / (yres/2.0f);
      k = 0;
      do
	{
	  temp = (zreal * zreal) - (zimag * zimag) + creal;
	  zimag = (2.0f * zreal * zimag) + cimag;
	  zreal = temp;
	  lengthsq = (zreal * zreal) + (zimag * zimag);
	  ++k;
	}
      while(lengthsq < 4.f && k < MAX_ITERS);
      if(k==MAX_ITERS)
	k = 0;
      float intensity = k / (float)MAX_ITERS;
      // simple red shading based on Mandelbrot function
      Color color(intensity, 0.f, 0.f); 

      // write the pixel to the framebuffer
      // see Image helper class
      image.set(i, j, color);
    }
}
