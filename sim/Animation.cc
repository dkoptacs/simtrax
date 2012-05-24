#include "Animation.h"

Animation::Animation(char const *keyframe, int n_frames, FourByte *mem, int n_nodes,
	  std::vector<int> *tri_orders, int num_tris, int start_tris, int s_nodes, int start_alt_tris, int start_alt_nodes)
{
  first_keyframe = keyframe;
  num_frames = n_frames;
  memory = mem;
  triangle_orders = tri_orders;
  start_triangles = start_tris;
  start_seperate_triangles = start_alt_tris;
  start_nodes = s_nodes;
  start_seperate_nodes = start_alt_nodes;
  num_triangles = num_tris;
  num_nodes = n_nodes;
  current_frame = 0;
  num_keyframes = 0;
  segregate_update = (start_seperate_triangles != -1);

  // figure out the model name and number of first keyframe
  strcpy(model_name, first_keyframe);
  int num_length = strspn(model_name + strcspn(model_name, "1234567890"), "1234567890");
  char *extention = strstr(model_name, ".obj");
  *extention = '\0';
  extention -= num_length;
  current_keyframe = atoi(extention);
  *extention = '\0';
  

  // count the number of keyframes
  FILE *input;
  int temp_framenum = current_keyframe;
  strcpy(next_filename, first_keyframe);
  while((input = fopen(next_filename, "r")))
    {
      getNextKeyFrame(next_filename, ++temp_framenum);
      num_keyframes++;
      fclose(input);
    }

  printf("found %d keyframes\n", num_keyframes);

  if(num_frames < num_keyframes)
    {
      num_keyframes = num_frames;
    }
  
  framestep = 1.0f/(num_frames - 1);
  keystep = 1.0f/(num_keyframes - 1);
  next_switch = keystep;
  progress = 0.0f;
  frames_this_phase = (int)((next_switch - progress) / framestep)+1;
  translate = 0.0f;

  if(num_keyframes < 2)
    {
      perror("error: animation must have at least 2 keyframes\n");
      exit(1);
    }
  
  int dummy_loc = 0;
  OBJLoader::LoadModel(keyframe, &current_triangles, &empty_mats, (FourByte*)NULL, 0, dummy_loc);
  current_keyframe++;
  getNextKeyFrame(next_filename, current_keyframe);
  OBJLoader::LoadModel(next_filename, &next_triangles, &empty_mats, (FourByte*)NULL, 0, dummy_loc);

  if(segregate_update)
    {
      // keep track of the offset between to two separate sets of triangles
      triangle_offset = start_seperate_triangles - start_triangles;
      // swap the render array with the rotate array
      int temp = start_triangles;
      start_triangles = start_seperate_triangles;
      start_seperate_triangles = temp;
    }
   
}

int Animation::loadNextFrame()
{
  if(++current_frame == num_frames)
    return 0;

  

  if(segregate_update)
    {
      // copy the newly rotated bvh to the other array (so that the next rotation gets the previous rotations)
      copyBVH();
      
      // swap the render array with the rotate array
      int temp = start_triangles;
      start_triangles = start_seperate_triangles;
      start_seperate_triangles = temp;
      // swap the render bvh pointer with the rotate bvh pointer
      temp = memory[8].ivalue;
      memory[8].ivalue = memory[24].ivalue;
      memory[24].ivalue = temp;
      
      //loadNextFrame(); // need to preemptively load the next frame when separate parts of the chip are working on separate frames
    }



  translate += (1.0f / (float)frames_this_phase);
  progress += framestep;


  printf("loading frame %d, keyframe = %d, progress = %f, framestep = %f, keystep = %f, frames_this_phase = %d\n", 
	 current_frame, current_keyframe, progress, framestep, keystep, frames_this_phase);
  printf("pushing triangles by %2.2f%%\n", translate * 100);


  // upload the triangles to the device memory  

  for(int i=0; i < num_triangles; i++)
    {
      for(int j=0; j < 3; j++)
	{

	  // Triangles get reordered in the BVH builder, but we have them in the order in which they were loaded from the keyframes,
	  // so we need to know their original BVH orders (triangle_orders)

	  // update p0
	  memory[start_triangles + (i * 11) + j + 0].fvalue = 
	    static_cast<float>( current_triangles.at(triangle_orders->at(i))->p0[j] + 
	    ((next_triangles.at(triangle_orders->at(i))->p0[j] - 
	      current_triangles.at(triangle_orders->at(i))->p0[j]) * translate) );
	  
	  // update p1
	  memory[start_triangles + (i * 11) + j + 3].fvalue = 
	    static_cast<float>( current_triangles.at(triangle_orders->at(i))->p1[j] +
	    ((next_triangles.at(triangle_orders->at(i))->p1[j] - 
	      current_triangles.at(triangle_orders->at(i))->p1[j]) * translate) );
	  
	  // update p2
	  memory[start_triangles + (i * 11) + j + 6].fvalue = 
	    static_cast<float>( current_triangles.at(triangle_orders->at(i))->p2[j] + 
	    ((next_triangles.at(triangle_orders->at(i))->p2[j] - 
	      current_triangles.at(triangle_orders->at(i))->p2[j]) * translate) );
	  
	}
    }

  if(current_frame < num_frames - 1 && current_keyframe < num_keyframes && progress >= next_switch)
    {
      printf("switching keyframes\n");
      next_switch += keystep;
      frames_this_phase = (int)((next_switch - progress) / framestep)+1;
      translate = 0.0f;
      int dummy_loc;
      // update the current positions
      for(size_t k = 0; k < current_triangles.size(); k++)
	{
	  delete(current_triangles.at(k));
	  delete(next_triangles.at(k));
	}
      current_triangles.clear();
      next_triangles.clear();
      
      OBJLoader::LoadModel(next_filename, &current_triangles, &empty_mats, (FourByte*)NULL, 0, dummy_loc);
      current_keyframe++;
      getNextKeyFrame(next_filename, current_keyframe);
      // load the next positions
      OBJLoader::LoadModel(next_filename, &next_triangles, &empty_mats, (FourByte*)NULL, 0, dummy_loc);
    }

  /*
  if(segregate_update)
    {
      // swap the render array with the rotate array
      int temp = start_triangles;
      start_triangles = start_seperate_triangles;
      start_seperate_triangles = temp;
      // swap the render bvh pointer with the rotate bvh pointer
      temp = memory[8].ivalue;
      memory[8].ivalue = memory[24].ivalue;
      memory[24].ivalue = temp;
    }
  */


  return 1;
}

void Animation::copyBVH()
{
  for(int i=0; i < num_nodes; i++)
    {
      memory[memory[8].ivalue + (i * 8) + 0].fvalue = memory[memory[24].ivalue + (i * 8) + 0].fvalue;
      memory[memory[8].ivalue + (i * 8) + 1].fvalue = memory[memory[24].ivalue + (i * 8) + 1].fvalue;
      memory[memory[8].ivalue + (i * 8) + 2].fvalue = memory[memory[24].ivalue + (i * 8) + 2].fvalue;
      memory[memory[8].ivalue + (i * 8) + 3].fvalue = memory[memory[24].ivalue + (i * 8) + 3].fvalue;
      memory[memory[8].ivalue + (i * 8) + 4].fvalue = memory[memory[24].ivalue + (i * 8) + 4].fvalue;
      memory[memory[8].ivalue + (i * 8) + 5].fvalue = memory[memory[24].ivalue + (i * 8) + 5].fvalue;
      memory[memory[8].ivalue + (i * 8) + 6].ivalue = memory[memory[24].ivalue + (i * 8) + 6].ivalue;

      // For leaf nodes, child pointers are direct memory locations (not IDs), need them to point to the correct set of triangles
      if(memory[memory[24].ivalue + (i * 8) + 6].ivalue > 0) // leaf node
	{
	  if(memory[8].ivalue > memory[24].ivalue)
	    memory[memory[8].ivalue + (i * 8) + 7].ivalue = memory[memory[24].ivalue + (i * 8) + 7].ivalue + triangle_offset;
	  else
	    memory[memory[8].ivalue + (i * 8) + 7].ivalue = memory[memory[24].ivalue + (i * 8) + 7].ivalue - triangle_offset;
	}
      else
	memory[memory[8].ivalue + (i * 8) + 7].ivalue = memory[memory[24].ivalue + (i * 8) + 7].ivalue;
    }
}

void Animation::getNextKeyFrame(char *name, int cur_framenum)
{
  sprintf(name, "%s%d.obj", model_name, cur_framenum);
}
