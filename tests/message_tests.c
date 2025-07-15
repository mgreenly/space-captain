#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "vendor/unity.h"

#include "../src/message.h"

// Function prototypes
void setUp(void);
void tearDown(void);
void test_message_type_to_string(void);
void test_message_header_size(void);
void test_ping_pong_message_size(void);
void test_message_type_ranges(void);

// Test that message_type_to_string returns correct strings
void test_message_type_to_string(void) {
  // Test Client-to-Server messages
  TEST_ASSERT_EQUAL_STRING("DIAL_UPDATE", message_type_to_string(MSG_DIAL_UPDATE));
  TEST_ASSERT_EQUAL_STRING("MOVEMENT_INPUT", message_type_to_string(MSG_MOVEMENT_INPUT));
  TEST_ASSERT_EQUAL_STRING("FIRE_WEAPON", message_type_to_string(MSG_FIRE_WEAPON));
  TEST_ASSERT_EQUAL_STRING("STATE_ACK", message_type_to_string(MSG_STATE_ACK));
  TEST_ASSERT_EQUAL_STRING("HEARTBEAT", message_type_to_string(MSG_HEARTBEAT));
  TEST_ASSERT_EQUAL_STRING("PING", message_type_to_string(MSG_PING));

  // Test Server-to-Client messages
  TEST_ASSERT_EQUAL_STRING("STATE_UPDATE", message_type_to_string(MSG_STATE_UPDATE));
  TEST_ASSERT_EQUAL_STRING("ENTITY_DESTROYED", message_type_to_string(MSG_ENTITY_DESTROYED));
  TEST_ASSERT_EQUAL_STRING("DAMAGE_RECEIVED", message_type_to_string(MSG_DAMAGE_RECEIVED));
  TEST_ASSERT_EQUAL_STRING("ERROR_RESPONSE", message_type_to_string(MSG_ERROR_RESPONSE));
  TEST_ASSERT_EQUAL_STRING("PONG", message_type_to_string(MSG_PONG));

  // Test Connection Management messages
  TEST_ASSERT_EQUAL_STRING("CONNECTION_ACCEPTED", message_type_to_string(MSG_CONNECTION_ACCEPTED));
  TEST_ASSERT_EQUAL_STRING("CONNECTION_REJECTED", message_type_to_string(MSG_CONNECTION_REJECTED));
  TEST_ASSERT_EQUAL_STRING("DISCONNECT_NOTIFY", message_type_to_string(MSG_DISCONNECT_NOTIFY));

  // Test unknown message type
  TEST_ASSERT_EQUAL_STRING("UNKNOWN", message_type_to_string((message_type_t) 0xFFFF));
}

// Test struct sizes match protocol specification
void test_message_header_size(void) {
  // According to PRD Section 5, the message header should be:
  // - protocol_version: uint16_t (2 bytes)
  // - message_type: uint16_t (2 bytes)
  // - sequence_number: uint32_t (4 bytes)
  // - timestamp: uint64_t (8 bytes)
  // - payload_length: uint16_t (2 bytes)
  // Total: 18 bytes

  TEST_ASSERT_EQUAL_UINT(18, sizeof(message_header_t));

  // Verify individual field offsets
  message_header_t header;
  TEST_ASSERT_EQUAL_UINT(0, (size_t) &header.protocol_version - (size_t) &header);
  TEST_ASSERT_EQUAL_UINT(2, (size_t) &header.message_type - (size_t) &header);
  TEST_ASSERT_EQUAL_UINT(4, (size_t) &header.sequence_number - (size_t) &header);
  TEST_ASSERT_EQUAL_UINT(8, (size_t) &header.timestamp - (size_t) &header);
  TEST_ASSERT_EQUAL_UINT(16, (size_t) &header.payload_length - (size_t) &header);
}

// Test PING and PONG message structures
void test_ping_pong_message_size(void) {
  // PING and PONG messages should only contain the header
  TEST_ASSERT_EQUAL_UINT(sizeof(message_header_t), sizeof(ping_message_t));
  TEST_ASSERT_EQUAL_UINT(sizeof(message_header_t), sizeof(pong_message_t));
}

// Test message type ranges
void test_message_type_ranges(void) {
  // Client-to-Server messages should be in range 0x0000-0x0FFF
  TEST_ASSERT_TRUE(MSG_DIAL_UPDATE >= 0x0000 && MSG_DIAL_UPDATE <= 0x0FFF);
  TEST_ASSERT_TRUE(MSG_MOVEMENT_INPUT >= 0x0000 && MSG_MOVEMENT_INPUT <= 0x0FFF);
  TEST_ASSERT_TRUE(MSG_FIRE_WEAPON >= 0x0000 && MSG_FIRE_WEAPON <= 0x0FFF);
  TEST_ASSERT_TRUE(MSG_STATE_ACK >= 0x0000 && MSG_STATE_ACK <= 0x0FFF);
  TEST_ASSERT_TRUE(MSG_HEARTBEAT >= 0x0000 && MSG_HEARTBEAT <= 0x0FFF);
  TEST_ASSERT_TRUE(MSG_PING >= 0x0000 && MSG_PING <= 0x0FFF);

  // Server-to-Client messages should be in range 0x1000-0x1FFF
  TEST_ASSERT_TRUE(MSG_STATE_UPDATE >= 0x1000 && MSG_STATE_UPDATE <= 0x1FFF);
  TEST_ASSERT_TRUE(MSG_ENTITY_DESTROYED >= 0x1000 && MSG_ENTITY_DESTROYED <= 0x1FFF);
  TEST_ASSERT_TRUE(MSG_DAMAGE_RECEIVED >= 0x1000 && MSG_DAMAGE_RECEIVED <= 0x1FFF);
  TEST_ASSERT_TRUE(MSG_ERROR_RESPONSE >= 0x1000 && MSG_ERROR_RESPONSE <= 0x1FFF);
  TEST_ASSERT_TRUE(MSG_PONG >= 0x1000 && MSG_PONG <= 0x1FFF);

  // Connection Management messages should be in range 0x2000-0x2FFF
  TEST_ASSERT_TRUE(MSG_CONNECTION_ACCEPTED >= 0x2000 && MSG_CONNECTION_ACCEPTED <= 0x2FFF);
  TEST_ASSERT_TRUE(MSG_CONNECTION_REJECTED >= 0x2000 && MSG_CONNECTION_REJECTED <= 0x2FFF);
  TEST_ASSERT_TRUE(MSG_DISCONNECT_NOTIFY >= 0x2000 && MSG_DISCONNECT_NOTIFY <= 0x2FFF);
}

void setUp(void) {
  // Nothing to set up
}

void tearDown(void) {
  // Nothing to tear down
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_message_type_to_string);
  RUN_TEST(test_message_header_size);
  RUN_TEST(test_ping_pong_message_size);
  RUN_TEST(test_message_type_ranges);

  return UNITY_END();
}