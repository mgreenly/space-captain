#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

// Message types for v0.1.0 protocol
// Client-to-Server: 0x0000-0x0FFF
// Server-to-Client: 0x1000-0x1FFF
// Connection Mgmt:  0x2000-0x2FFF
typedef enum {
  // Client-to-Server messages
  MSG_DIAL_UPDATE    = 0x0001,
  MSG_MOVEMENT_INPUT = 0x0002,
  MSG_FIRE_WEAPON    = 0x0003,
  MSG_STATE_ACK      = 0x0004,
  MSG_HEARTBEAT      = 0x0005,

  // For initial protocol testing (not in PRD, but useful for iteration 2)
  MSG_PING = 0x0006,

  // Server-to-Client messages
  MSG_STATE_UPDATE     = 0x1001,
  MSG_ENTITY_DESTROYED = 0x1002,
  MSG_DAMAGE_RECEIVED  = 0x1003,
  MSG_ERROR_RESPONSE   = 0x1004,

  // For initial protocol testing (not in PRD, but useful for iteration 2)
  MSG_PONG = 0x1005,

  // Connection Management messages
  MSG_CONNECTION_ACCEPTED = 0x2001,
  MSG_CONNECTION_REJECTED = 0x2002,
  MSG_DISCONNECT_NOTIFY   = 0x2003
} message_type_t;

// Message header format per PRD Section 5
typedef struct __attribute__((packed)) {
  uint16_t protocol_version; // Protocol version (0x0001 for v0.1.0)
  uint16_t message_type;     // Message type from enum above
  uint32_t sequence_number;  // For ordering and acknowledgment
  uint64_t timestamp;        // Unix timestamp in milliseconds
  uint16_t payload_length;   // Size of the message payload
} message_header_t;

// Generic message structure
typedef struct {
  message_header_t header;
  uint8_t *payload; // Dynamically allocated payload
} message_t;

// PING message (header only for initial testing)
typedef struct __attribute__((packed)) {
  message_header_t header;
} ping_message_t;

// PONG message (header only for initial testing)
typedef struct __attribute__((packed)) {
  message_header_t header;
} pong_message_t;

// Utility function to convert message type to string
const char *message_type_to_string(message_type_t type);

#endif // MESSAGE_H