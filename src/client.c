#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "log.h"

#include "queue.c"

#define PORT 4242
#define HOST "127.0.0.1"

#define QUEUE_CAPACITY 256

//
// main
//
int32_t main(void) {
  queue_t* msg_queue = queue_create(QUEUE_CAPACITY);
  if (!msg_queue) {
    log_error("%s", "Failed to create message queue. Exiting.");
    return EXIT_FAILURE;
  }

  char *greeting = "Hello from the client!";

  queue_push(msg_queue, greeting);

  puts(queue_pop(msg_queue));

  queue_destroy(msg_queue);


  return EXIT_SUCCESS;
}

