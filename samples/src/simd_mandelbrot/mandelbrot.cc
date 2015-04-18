#include "trax.hpp"
#include "Image.h"

// Created by Will Usher
// SIMD vectorized version of mandelbrot sample using MIPS MSA instructions.

#define MAX_ITERS 100


int movemask(v4i32 mask){
  const v4i32 pop = __builtin_msa_pcnt_w(mask);
  const int *p = (int*)&pop;
  int mv_mask = 0;
  for (int i = 0; i < 4; ++i){
    if (p[i] == 32){
      mv_mask = mv_mask | 1 << i;
    }
  }
  return mv_mask;
}

void trax_main(){
  // Helper function to get a pointer to the framebuffer
  int start_fb = GetFrameBuffer();
  
  int xres = GetXRes();
  int yres = GetYRes();
  v4i32 vxres = __builtin_msa_fill_w(xres);
  v4i32 vyres = __builtin_msa_fill_w(yres);
  
  v4f32 fxres = __builtin_msa_ffint_s_w(vxres);
  v4f32 fyres = __builtin_msa_ffint_s_w(vyres);
  
  // set up the frame buffer
  Image image(xres, yres, start_fb);
  
  // Pre-compute some frequently re-used constants
  const v4f32 inv_1_4 = __builtin_msa_frcp_w(v4f32{1.4f, 1.4f, 1.4f, 1.4f});
  const v4f32 inv_2 = __builtin_msa_frcp_w(v4f32{2.f, 2.f, 2.f, 2.f});
  const v4f32 x_denom = __builtin_msa_frcp_w(fxres * v4f32{0.5f, 0.5f, 0.5f, 0.5f});
  const v4f32 y_denom = __builtin_msa_frcp_w(fyres * v4f32{0.5f, 0.5f, 0.5f, 0.5f});
  
  // Compute Mandelbrot on 4 pixel blocks at a time
  for (int pix = atomicinc(0) * 4; pix < xres * yres; pix = atomicinc(0) * 4){
    v4i32 vpix = {pix, pix + 1, pix + 2, pix + 3};
    v4i32 i = vpix / vxres;
    v4i32 j = vpix % vxres;
    
    v4f32 f_i = __builtin_msa_ffint_s_w(i);
    v4f32 f_j = __builtin_msa_ffint_s_w(j);
    
    
    v4f32 zreal = v4f32{0.f, 0.f, 0.f, 0.f};
    v4f32 zimag = v4f32{0.f, 0.f, 0.f, 0.f};
    v4f32 creal = (f_j - fxres * inv_1_4) * x_denom;
    v4f32 cimag = (f_i - fyres * inv_2) * y_denom;
    
    v4i32 k = __builtin_msa_fill_w(0);
    v4f32 lengthsq = (zreal * zreal) + (zimag * zimag);
    const v4f32 max_length = v4f32{4.f, 4.f, 4.f, 4.f};
    const v4i32 max_iters = __builtin_msa_fill_w(MAX_ITERS);

    // Compute a mask of active pixels that we need to loop over and
    // execute the loop until all are completed
    v4i32 mask = __builtin_msa_and_v(__builtin_msa_fclt_w(lengthsq, max_length),
				     __builtin_msa_clt_s_w(k, max_iters));
    
    while (movemask(mask) != 0){
      
      // Compute (zreal * zreal) - (zimag * zimag) + creal in two parts here
      // first do the tail end with an fmadd then do the first bit minus the tail
      // with an fmsub.
      v4f32 temp = __builtin_msa_fmadd_w(creal, zreal, zreal);
      temp = __builtin_msa_fmsub_w(temp, zimag, zimag);
      v4f32 zimag_tmp = (v4f32{2.f, 2.f, 2.f, 2.f} * zreal * zimag) + cimag;
      
      zimag = __builtin_msa_bsel_v(mask, zimag, zimag_tmp);
      zreal = __builtin_msa_bsel_v(mask, zreal, temp);
      
      lengthsq = __builtin_msa_bsel_v(mask, lengthsq, (zreal * zreal) + (zimag * zimag));
      
      k = __builtin_msa_bsel_v(mask, k, k + __builtin_msa_fill_w(1));
      
      // Update our execution mask, some may have finished
      mask = __builtin_msa_and_v(__builtin_msa_fclt_w(lengthsq, max_length),
				 __builtin_msa_clt_s_w(k, max_iters));
      
    }
    // Compute mask of pixels that hit the max number of iterations and set k to 0
    // for those elements
    mask = __builtin_msa_ceq_w(k, max_iters);
    k = __builtin_msa_bsel_v(mask, k, __builtin_msa_fill_w(0));
    
    v4f32 intensity = __builtin_msa_ffint_s_w(k) / __builtin_msa_ffint_s_w(max_iters);

    // Now serialize over those pixels that are in the framebuffer (eg. some may be out of bounds)
    // and write the pixel values
    for (int i = 0; i < 4; ++i){
      Color color(intensity[i], 0.f, 0.f); 
      // Just write pixels that are in bounds, if the image size wasn't a multiple of 4
      // the last block of pixels may have some out of bounds
      if ((pix + i) < (xres * yres)){
	image.set((pix + i) / xres, (pix + i) % xres, color);
      }
    }
  }
}
