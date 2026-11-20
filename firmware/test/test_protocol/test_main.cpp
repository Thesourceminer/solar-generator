#include <string.h>
#include <unity.h>
#include "protocol.h"
#include "vectors_gen.h"

void setUp() {}
void tearDown() {}

static void test_valid_status_vectors_encode_byte_exact() {
  for (const auto& v : kStatusVectors) {
    if (!v.valid) continue;
    StatusRecord r;
    v.expect(r);
    uint8_t out[kStatusLen];
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(kStatusLen, encodeStatus(r, out), v.name);
    TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(v.bytes, out, kStatusLen, v.name);
  }
}

static void test_valid_status_vectors_decode_to_expected() {
  for (const auto& v : kStatusVectors) {
    if (!v.valid) continue;
    StatusRecord want;
    v.expect(want);
    StatusRecord got;
    TEST_ASSERT_TRUE_MESSAGE(decodeStatus(v.bytes, v.len, got), v.name);
    TEST_ASSERT_TRUE_MESSAGE(got == want, v.name);
  }
}

static void test_invalid_status_vectors_rejected_and_output_untouched() {
  for (const auto& v : kStatusVectors) {
    if (v.valid) continue;
    StatusRecord got;
    got.seq = 1234;
    TEST_ASSERT_FALSE_MESSAGE(decodeStatus(v.bytes, v.len, got), v.name);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(1234, got.seq, v.name);
  }
}

static void test_command_vectors_parse_or_reject() {
  for (const auto& c : kCommandVectors) {
    Command cmd;
    TEST_ASSERT_EQUAL_MESSAGE(c.valid, parseCommand(c.bytes, c.len, cmd), c.name);
  }
}

static void test_circuit_command_fields() {
  const uint8_t b[] = {0x01, 0x03, 0x01};
  Command c;
  TEST_ASSERT_TRUE(parseCommand(b, 3, c));
  TEST_ASSERT_EQUAL_UINT8(OP_SET_CIRCUIT, c.op);
  TEST_ASSERT_EQUAL_UINT8(3, c.a);
  TEST_ASSERT_EQUAL_UINT8(1, c.b);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_valid_status_vectors_encode_byte_exact);
  RUN_TEST(test_valid_status_vectors_decode_to_expected);
  RUN_TEST(test_invalid_status_vectors_rejected_and_output_untouched);
  RUN_TEST(test_command_vectors_parse_or_reject);
  RUN_TEST(test_circuit_command_fields);
  return UNITY_END();
}
