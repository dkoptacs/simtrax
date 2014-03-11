
#ifndef HITRECORD_H
#define HITRECORD_H

#define EPSILON (0.0000001f)

// This class keeps track of ray hit information such as distance, barycentric coordinates, object ID

class HitRecord {
 public:
  HitRecord()
    {
      obj_id = 0;
      min = 10000000.f;
      did_hit = false;
      hit_inside = false;
    }

  HitRecord(const float _max) {
    obj_id = 0;
    min = _max;
    did_hit = false;
    hit_inside = false;
  }

  HitRecord& operator=(const HitRecord& copy) {

    obj_id = copy.obj_id;
    min = copy.min;
    did_hit = copy.did_hit;
    hit_inside = copy.hit_inside;
    return *this;
  }

  inline bool hit(const float &t, const int &id, const bool &inside = false);

  inline float minT() const {
    return min;
  }
  inline int getID() const {
    return obj_id;
  }
  inline bool didHit() const {
    return did_hit;
  }
  inline bool didHitInside() const {
    return hit_inside;
  }
  inline void setBaryCoords(float _b1, float _b2)
  {
    b1 = _b1;
    b2 = _b2;
  }
  inline float getB1() const
  {
    return b1; 
  }
  inline float getB2() const
  {
    return b2; 
  }
  
 private:
  float b1; // barycentric coordinates of hit triangle
  float b2;
  float min; // distance to closest hit object
  int obj_id;
  bool did_hit;
  bool hit_inside;
};

inline bool HitRecord::hit(const float &t, const int &id, const bool &inside) {
  if(t > EPSILON && t < min){
    min = t;
    obj_id = id;
    did_hit = true;
    hit_inside = inside;
    return true;
  } else {
    return false;
  }
}


#endif
