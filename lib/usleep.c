#include <unistd.h>
#include "sleep.h"

/* usleep(3)
   CE3TSK 2026-09-28: the argument is a Fortran default INTEGER, 4 bytes. It was unsigned long,
   8 bytes on Linux and macOS, so its upper half came from past the caller's integer - harmless
   only because usleep() takes 32 bits and the compiler loaded just the lower half. */
void usleep_(int *microsec)
{
  usleep(*microsec);
}
