#include <stdlib.h>
#include <stdio.h>


#include "log.h"



//
// main
//
int32_t main(void) {
  log_info("%s", "doing stuff in main");

  puts("Hello from the client!");
  return EXIT_SUCCESS;
}

