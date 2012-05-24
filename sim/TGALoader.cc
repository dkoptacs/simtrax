#include "TGALoader.h"

// Windows pipe stuff (needs stdio.h)
#ifdef WIN32
# ifndef popen
#	define popen _popen
#	define pclose _pclose
# endif
#endif

// taken from http://steinsoft.net/index.php?site=Programming/Code%20Snippets/Cpp/no8

// reads "filename" tga file
bool loadTGA(const char *filename, STGA* tgaFile)
{
  
  char command_buf[512];
  
  // ":-" means write output to stdout
  sprintf(command_buf, "convert %s tga:-", filename);
  // run "convert" to get a tga, and redirect the output to FILE* file
  FILE* file = popen(command_buf, "r");
  
  /*
  // find the extension of the image file
  char * tmp = NULL, *tmp2 = NULL;
  tmp2 = strstr(filename, ".");
  while (tmp2 != NULL) {
    tmp = tmp2++;
    tmp2 = strstr(tmp2, ".");
  }
  
  if(strcmp(tmp, ".tga") == 0) // image is already tga, don't need to convert
  
  printf("image type: %s\n", tmp);

  FILE* file = fopen(filename, "rb");
  */
  if(!file)
    {
      printf("Can't find texture: %s\n", filename);
      return false;
    }

  printf("Loading texture: %s\n", filename);

  unsigned char type[4];
  unsigned char info[6];
  unsigned char throwaway[10];
  
  if (!file)
    return false;
  
  fread (&type, sizeof (char), 3, file);
  // can't use fseek on popen (doesn't support binary "b" open mode)
  fread (&throwaway, sizeof (char), 9, file); 
  //fseek (file, 12, SEEK_SET);
  fread (&info, sizeof (char), 6, file);
  
  //image type either 2 (color) or 3 (greyscale)
  if (type[1] != 0 || (type[2] != 2 && type[2] != 3))
    {
      printf("unsupported texture image type\n");
      pclose(file);
      return false;
    }
  
  tgaFile->width = info[0] + info[1] * 256;
  tgaFile->height = info[2] + info[3] * 256;
  tgaFile->byteCount = info[4] / 8;

  // Support grayscale, rgb, rgba
  if (tgaFile->byteCount == 2 || tgaFile->byteCount > 4) {
    printf("unsupported texture color depth: %d bytes/pixel\n", tgaFile->byteCount);
    pclose(file);
    return false;
  }
  
  //tgaFile->print();
  
  // there was an error here in code from URL listed above, had an extra (* width) in there
  long imageSize = tgaFile->width * tgaFile->height * tgaFile->byteCount;
  
  //allocate memory for image data
  tgaFile->data = new unsigned char[imageSize];
  
  //read in image data
  fread(tgaFile->data, sizeof(unsigned char), imageSize, file);
  
  //close file
  pclose(file);
  
  return true;
}
