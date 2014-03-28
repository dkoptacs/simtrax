
#ifndef MATH_H
#define MATH_H

static inline float Fabs(float x) {
  return x < 0.0f ? -x : x;
}

static inline int Floor(float d)
{
  if(d<0.f){
    int i = -static_cast<int>(-d);
    if(i == d)
      return i;
    else
      return i-1;
  } else {
    return static_cast<int>(d);
  }
}

#endif
