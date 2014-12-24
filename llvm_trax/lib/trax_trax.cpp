#include "trax.hpp"
#include <stdarg.h>

int main() {
  trax_main();
  return 0;
}

extern int printf ( const char * format, ... )
{

  // Arguments will all be placed on the stack in subsequent words.
  // Just need to pass the address of the first arg to the intrinsic
  trax_printformat(&format);

  // Return something just to match the signature of stdio::printf
  // Ideally this would return the number of args successfully printed
  return 0;

}
