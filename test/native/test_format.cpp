#include <string.h>

#include <unity.h>

#include "SystemChrono/SystemChrono.h"

using namespace SystemChrono;

static void test_format_time_zero() {
  char buf[kFormatTimeBufferSize];
  const Status st = formatTimeToBuffer(0, buf, sizeof(buf));
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_STRING("0:00:00.000", buf);
}

static void test_format_time_negative() {
  char buf[kFormatTimeBufferSize];
  const Status st = formatTimeToBuffer(-1234567, buf, sizeof(buf));
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_STRING("-0:00:01.234", buf);
}

static void test_format_time_buffer_too_small() {
  char buf[kFormatTimeBufferSize - 1];
  const Status st = formatTimeToBuffer(0, buf, sizeof(buf));
  TEST_ASSERT_FALSE(st.ok());
  TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(Err::INVALID_CONFIG),
                           static_cast<uint16_t>(st.code));
}

static void test_format_now() {
  char buf[kFormatTimeBufferSize];
  const Status st = formatNowToBuffer(buf, sizeof(buf));
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(strlen(buf) > 0);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_format_time_zero);
  RUN_TEST(test_format_time_negative);
  RUN_TEST(test_format_time_buffer_too_small);
  RUN_TEST(test_format_now);
  return UNITY_END();
}
