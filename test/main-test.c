#include "unity.h"

extern void run_semver_tests();
extern void run_semverreq_tests();

void setUp(void) {}

void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();

  run_semver_tests();

  return UNITY_END();
}
