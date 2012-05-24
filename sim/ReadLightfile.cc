#include "ReadLightfile.h"
#include <stdio.h>
#include <stdlib.h>


void ReadLightfile::LoadFile(const char* filename, float* light_pos) {
  FILE* input = fopen(filename, "r");
  if (!input) {
    perror("Failed to open light file.");
    exit(1);
  }

  if (fscanf(input, "%f %f %f",
	     &light_pos[0], &light_pos[1], &light_pos[2]) != 3)
    perror("Error reading light file.\n");

  fclose(input);
}

