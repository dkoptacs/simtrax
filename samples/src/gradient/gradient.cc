#include "trax.hpp"

// Utility function to store a color at a pixel offset in the frame buffer
inline void StorePixel(const int &fb, const int &pixel, const float &r, const float &g, const float &b) {
  storef(r, fb + pixel*3, 0);
  storef(g, fb + pixel*3, 1);
  storef(b, fb + pixel*3, 2);
}

void trax_main()
{
  // Load the pointer to the frame buffer in global memory
  int framebuffer = GetFrameBuffer();

  // Load the screen size
  int width = GetXRes();
  int height = GetYRes();

  // Iterate over all of the pixels
  // Using atomicinc here allows your code to have only one thread execute per return value
  for (int pixel = atomicinc(0); pixel < width * height; pixel = atomicinc(0)) {
    // Store a color based on screen location
    int i = pixel % height;
    int j = pixel / width;
    // framebuffer pointer, pixel offset, red, green, blue
    StorePixel( framebuffer, pixel, (float)j/height, (float)i/width, 0.0f );
  }

  // You can use barriers to synchronize, but make sure all threads get to them the same
  // number of times otherwise you could put the system into deadlock.
  // Note: For this example, and any example with independent writes, the barrier is not needed
  // nor is it helpful. It is only here for an example of the usage.
  barrier(5);

}


