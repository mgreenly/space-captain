// message.h

#pragma once

#include <stdint.h>

// Defines the types of messages that can be sent.
typedef enum {
    MSG_ECHO,
    MSG_REVERSE,
    MSG_TIME,
} MessageType;

// The fixed-size header for every message.
// __attribute__((packed)) is a GCC/Clang extension to prevent the compiler
// from adding padding between struct members, ensuring a consistent
// binary layout.
typedef struct __attribute__((packed)) {
    MessageType type;
    uint32_t length; // Length of the body (including null terminator)
} MessageHeader;

#define MAX_MSG_BODY_LEN 1024
#define MAX_MSG_LEN (sizeof(MessageHeader) + MAX_MSG_BODY_LEN)
