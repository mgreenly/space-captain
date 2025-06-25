#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

typedef enum {
  MSG_ECHO,
  MSG_REVERSE,
  MSG_TIME
} message_type_t;

typedef struct __attribute__((packed)) {
  uint32_t type;
  uint32_t length;
} message_header_t;

typedef struct {
  // int client_fd;
  message_header_t header;
  char *body;
} message_t;

#endif // MESSAGE_H
