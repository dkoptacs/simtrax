#ifndef _SIMHWRT_DISK_BUFFER_H_
#define _SIMHWRT_DISK_BUFFER_H_

#include <fstream>

template<typename T, typename H = void>
class DiskBuffer {
public:
  DiskBuffer(const char *filename, int _n) {
    n = _n;
    storage = new T[n];
    p = 0;
    out.open(filename);
    if (!out) {
      printf("File %s failed to open.\n", filename);
    }
  }

  // NOTE(choudhury): If no header type is specified (i.e. the default
  // of void is used) then the following function results in a
  // compile-time error if it is called.  This trick makes you select
  // an appropriate header type if you want to use this constructor to
  // write out a header.
  DiskBuffer(const char *filename, int _n, H header) {
    n = _n;
    storage = new T[n];
    p = 0;
    out.open(filename);
    if (!out) {
      printf("File %s failed to open.\n", filename);
    }
    out.write(reinterpret_cast<const char *>(&header), sizeof(H));
  }

  ~DiskBuffer() {
    Flush();
    out.close();
    delete [] storage;
  }
  
  void AddItem(const T&item) {
    storage[p] = item;
    p++;
    if (p == n) {
      Flush();
    }
  }
  void Flush(){
    out.write(reinterpret_cast<const char *>(storage), sizeof(T)*p);
    p = 0;
  }
  
private:
  std::ofstream out;
  T* storage;
  int p, n;
};

#endif // _SIMHWRT_DISK_BUFFER_H_
