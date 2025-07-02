#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

typedef enum {
  MSG_ECHO    = 0, // Echo message back to sender
  MSG_REVERSE = 1, // Reverse the message content
  MSG_TIME    = 2, // Return current server time
  MSG_INVALID = -1 // Invalid message type
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

// Utility function to convert message type to string
static inline const char *message_type_to_string(message_type_t type) {
  switch (type) {
  case MSG_ECHO:
    return "ECHO";
  case MSG_REVERSE:
    return "REVERSE";
  case MSG_TIME:
    return "TIME";
  case MSG_INVALID:
  default:
    return "UNKNOWN";
  }
}

#endif // MESSAGE_H
