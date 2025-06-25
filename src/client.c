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
  queue_t *msg_queue = queue_create(QUEUE_CAPACITY);
  if (!msg_queue) {
    log_error("%s", "Failed to create message queue. Exiting.");
    return EXIT_FAILURE;
  }
  // message_header_t msg_header = { MSG_ECHO, strlen(greeting) + 1 };

  char *msg_body = "Hello from the client!";
  message_t *msg = malloc(sizeof(message_t));
  msg->header.type = MSG_ECHO;
  msg->header.length = strlen(msg_body) + 1;
  msg->body = msg_body;

  printf("%d\n", msg->header.type);
  printf("%d\n", msg->header.length);
  printf("%s\n", msg->body);

  if (queue_add(msg_queue, msg) != QUEUE_SUCCESS) {
    log_error("%s", "Failed to add message to queue");
    free(msg);
    queue_destroy(msg_queue);
    return EXIT_FAILURE;
  }

  message_t *msg2 = NULL;
  if (queue_pop(msg_queue, &msg2) != QUEUE_SUCCESS) {
    log_error("%s", "Failed to pop message from queue");
    free(msg);
    queue_destroy(msg_queue);
    return EXIT_FAILURE;
  }

  printf("%s\n", msg2->body);

  free(msg);

  queue_destroy(msg_queue);

  return EXIT_SUCCESS;
}
