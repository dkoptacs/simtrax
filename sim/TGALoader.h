#ifndef _SIMHWRT_TGA_H_
#define _SIMHWRT_TGA_H_

// taken from http://steinsoft.net/index.php?site=Programming/Code%20Snippets/Cpp/no8

#include <stdio.h>
#include <string.h>
#include <string>

// Windows pipe stuff (needs stdio.h)
#ifdef WIN32
# ifndef popen
#	define popen _popen
#	define pclose _pclose
# endif
#endif

// stores the image data
struct STGA
{
  STGA()
  {
    data = (unsigned char*)0;
    width = 0;
    height = 0;
    byteCount = 0;
    address = -1;
  }
  
  ~STGA() { delete[] data; data = 0; }
  
  void destroy() { delete[] data; data = 0; }
  
  void print()
  {
    printf("width: %d\n", width);
    printf("height: %d\n", height);
    printf("bytecount: %d\n", byteCount);
  }

  int width;
  int height;
  unsigned char byteCount;
  unsigned char* data;
  std::string name;
  int address;
};

// reads "filename" tga file
bool loadTGA(const char *filename, STGA* tgaFile);

#endif // define _SIMHWRT_TGA_H_
