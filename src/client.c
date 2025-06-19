#include <stdlib.h>
#include <stdio.h>


#include "log.h"

#include "queue.c"

#define PORT 4242
#define HOST "127.0.0.1"

//
// main
//
int32_t main(void) {
  log_info("%s", "doing stuff in main");

  puts("Hello from the client!");
  return EXIT_SUCCESS;
}

