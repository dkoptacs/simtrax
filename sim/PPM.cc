#include "PPM.h"
#include <fstream>
#include <cstdlib>
#include <sstream>

PPM::PPM(std::string& filename)
{
  std::ifstream in(filename.c_str());
  if (in.fail())
    {
	  std::cerr << "Error opening file " << filename.c_str() << std::endl;
      exit(-1);
    }
  std::string type, tmpStr;
  std::getline( in, tmpStr );
  std::stringstream tmpStream( tmpStr );
  int maxval;
//  in >> type;
//  in >> width >> height >> maxval;
  tmpStream >> type;
  tmpStream >> width >> height >> maxval;
  if (in.fail())
    {
	  std::cerr << "error 1 reading " << filename.c_str() << std::endl;
      exit(-1);
    }
  std::cerr << filename.c_str() << " width: " << width << " height: " << height << std::endl;
  in.get();//go to next line
  if (maxval > 255)
    {
      std::cerr << "pixel values too big." << std::endl;
      exit(-1);
    }
  char* pixels = (char*)malloc(sizeof(char) * width * height * 3);
  in.read((char*)pixels, width*height*3);
  if (in.fail())
    {
      std::cerr << "error 2 reading " << filename.c_str() << std::endl;
      exit(-1);
    }
  in.close();
  data = (FourByte*)malloc(sizeof(FourByte) * width * height * 3);  
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      data[i * width * 3 + j * 3].ivalue = (int)pixels[i * width * 3 + j * 3];
      data[i * width * 3 + j * 3 + 1].ivalue = (int)pixels[i * width * 3 + j * 3 + 1];
      data[i * width * 3 + j * 3 + 2].ivalue = (int)pixels[i * width * 3 + j * 3 + 2];
    }
  }
  // free pixels
  // ...
}

void PPM::LoadIntoMemory(int& memory_position, int max_memory,
			 FourByte* memory) {
  memory[memory_position].ivalue = width;
  memory_position++;
  memory[memory_position].ivalue = height;
  memory_position++;
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      memory[memory_position] = data[i * width * 3 + j * 3];
      memory_position++;
      memory[memory_position] = data[i * width * 3 + j * 3 + 1];
      memory_position++;
      memory[memory_position] = data[i * width * 3 + j * 3 + 2];
      memory_position++;
    }
  }

}
