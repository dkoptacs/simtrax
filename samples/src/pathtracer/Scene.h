#ifndef SCENE_H
#define SCENE_H
class Scene
{ 
public:
  
  Scene(int start_bvh, int _start_materials, int _start_tex_coords, Light _light, Color _background)
  {
    bvh = BoundingVolumeHierarchy(start_bvh);
    start_materials = _start_materials;
    start_tex_coords = _start_tex_coords;
    light = _light;
    background = _background;
  }
  BoundingVolumeHierarchy bvh;
  int start_materials;
  int start_tex_coords;
  Light light; 
  Color background; 

};
#endif
