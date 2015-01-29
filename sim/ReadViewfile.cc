#include "ReadViewfile.h"
#include "Camera.h"
#include <stdio.h>
#include <stdlib.h>


simtrax::Camera* ReadViewfile::LoadFile(const char* filename, float far) {
  FILE* input = fopen(filename, "r");
  if (!input) {
    perror("Failed to open camera file.");
    exit(1);
  }

  Vector3f eye;
  Vector3f view;
  Vector3f up;

  float aperture;
  float left, right, bottom, top, distance;

  if (fscanf(input, "%f %f %f  %f %f %f  %f %f %f  %f  %f %f %f %f  %f",
         &eye.e[0], &eye.e[1], &eye.e[2],
         &view.e[0], &view.e[1], &view.e[2],
         &up.e[0], &up.e[1], &up.e[2],
         &aperture,
	     &left, &right, &bottom, &top, &distance) < 4)
    perror("Error reading view file.\n");

  fclose(input);
  return new simtrax::Camera(eye, view, up, aperture,
                             left, right, bottom, top,
                             distance, far);
}

