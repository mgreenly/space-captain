#include "message.h"

// Utility function to convert message type to string for logging and debugging
const char *message_type_to_string(message_type_t type) {
  switch (type) {
  // Client-to-Server messages
  case MSG_DIAL_UPDATE:
    return "DIAL_UPDATE";
  case MSG_MOVEMENT_INPUT:
    return "MOVEMENT_INPUT";
  case MSG_FIRE_WEAPON:
    return "FIRE_WEAPON";
  case MSG_STATE_ACK:
    return "STATE_ACK";
  case MSG_HEARTBEAT:
    return "HEARTBEAT";
  case MSG_PING:
    return "PING";

  // Server-to-Client messages
  case MSG_STATE_UPDATE:
    return "STATE_UPDATE";
  case MSG_ENTITY_DESTROYED:
    return "ENTITY_DESTROYED";
  case MSG_DAMAGE_RECEIVED:
    return "DAMAGE_RECEIVED";
  case MSG_ERROR_RESPONSE:
    return "ERROR_RESPONSE";
  case MSG_PONG:
    return "PONG";

  // Connection Management messages
  case MSG_CONNECTION_ACCEPTED:
    return "CONNECTION_ACCEPTED";
  case MSG_CONNECTION_REJECTED:
    return "CONNECTION_REJECTED";
  case MSG_DISCONNECT_NOTIFY:
    return "DISCONNECT_NOTIFY";

  default:
    return "UNKNOWN";
  }
}
