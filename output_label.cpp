#include <cstring>
#include <time.h>

#include "util.h"

int main(int argc, char **args) {
   struct timespec time;
   char buffer[1024 * 16];

   while (fgets(buffer, array_size(buffer), stdin)) {
      clock_gettime(CLOCK_MONOTONIC, &time);
      long long int itime = (long) time.tv_sec * 1000000000 + (long) time.tv_nsec;
      double ftime = itime / 1000000000.0;
      printf("[%3.9f] %s", ftime, buffer);
   }

   return 0;
}