#include <stdlib.h>
#include <string.h>

#include "semver.h"
#include "semverreq.h"

#include "unity.h"

/* white-box test */

typedef struct {
  const char *inp_lower;
  const int inp_lower_including;
  const char *inp_upper;
  const int inp_upper_including;

  const char *exp;
} exp1_t;

typedef struct {
  const char *inp;
  const char *exp;
} exp2_t;

void test_semverreq_print(void) {
  char buf[255];
  char dbuf[255];
  int k;
  semver_version_req r;
  semver_version *s1;
  semver_version *s2;
  size_t i;
  /* bound tests */
  exp1_t tests_single[] = {
      {"1.0.5", 0, 0, 0, ">1.0.5"},
      {"1.0.5", 1, 0, 0, ">=1.0.5"},
      {0, 0, "1.0.5", 0, "<1.0.5"},
      {0, 0, "1.0.5", 1, "<=1.0.5"},
      {"1.0.0", 0, "1.0.5", 0, ">1.0.0 <1.0.5"},
      {"1.0.0", 1, "1.0.5", 1, ">=1.0.0 <=1.0.5"},
      {"1.0.4", 1, "1.0.5", 1, ">=1.0.4 <=1.0.5"},
      {"1.0.5", 1, "1.0.5", 1, "=1.0.5"},
      {"1.0.5+build.id", 1, "1.0.5", 1, "=1.0.5+build.id"},
      {"1.0.5-pre+build.id", 1, "1.0.5-pre+build.id", 1, "=1.0.5-pre+build.id"}
  };

  /* empty input must yield empty string */

  r = semver_version_req_from(0, 0, 0, 0);
  TEST_ASSERT_NOT_NULL(r);

  k = semver_version_req_sprint(r, buf);
  TEST_ASSERT_EQUAL(k, 0);

  semver_version_req_delete(r);

  for (i = 0; i < sizeof(tests_single) / sizeof(exp1_t); i++) {
    s1 = NULL;
    if (tests_single[i].inp_lower != 0) {
      s1 = semver_version_from_string(tests_single[i].inp_lower);

      semver_version_sprint(s1, dbuf);
    }
    s2 = NULL;
    if (tests_single[i].inp_upper != 0) {
      s2 = semver_version_from_string(tests_single[i].inp_upper);

      semver_version_sprint(s2, dbuf);
    }

    r = semver_version_req_from(s1, tests_single[i].inp_lower_including,
                                s2, tests_single[i].inp_upper_including);
    TEST_ASSERT_NOT_NULL(r);

    k = semver_version_req_sprint(r, buf);

    TEST_ASSERT_EQUAL_STRING(tests_single[i].exp, (const char *)&buf);

    semver_version_req_delete(r);
    r = 0;

    if (s1) {
      semver_version_delete(s1);
      s1 = 0;
    }
    if (s2) {
      semver_version_delete(s2);
      s2 = 0;
    }
  }
}

void test_semverreq_invalid(void) {
  semver_version_req r, *p;
  semver_version *s1 = semver_version_from_string("1.2.3");
  semver_version *s2 = semver_version_from_string("1.2.2");

  /* upper < lower must be rejected */
  r = semver_version_req_from(s1, 0, s2, 0);
  TEST_ASSERT_EQUAL(r, 0);

  /* >1.2.2 <1.2.2 is not possible */
  p = semver_version_req_from(s1, 0, s1, 0);
  TEST_ASSERT_EQUAL(p, 0);
}

void test_semverreq_parse(void) {
  exp2_t tests[] = {
      {">=0.0.1 <1.0.0", ">=0.0.1 <1.0.0"},
      {"<1.0.0 >=0.0.1", ">=0.0.1 <1.0.0"},     /* handle swapped lower/upper bounds */
      {" >= 0.0.1, < 1.0.0  ", ">=0.0.1 <1.0.0"},
      {" >= 0.0.1; < 1.0.0  ", ">=0.0.1 <1.0.0"},
      {" >= 0.0.1 < 1.0.0  ", ">=0.0.1 <1.0.0"},
      {"=1.0.5", "=1.0.5"},
      {">1.0.5", ">1.0.5"},
      {">=1.0.5", ">=1.0.5"},
      {"<=2.0.0", "<=2.0.0"},
      {"  >=\t2.0.0   ", ">=2.0.0"},
      {">=1.3.9 <=1.3.9", "=1.3.9"},     /* simplify upper/lower in case of eq. */
      {"~1.4.3", ">=1.4.3 <1.5.0"},
      {"~1.4.3-some+build", ">=1.4.3-some+build <1.5.0"},
      {"~0.0.2", ">=0.0.2 <0.1.0"},
      {"~7.4.2+build", ">=7.4.2+build <7.5.0"},
      {"^1.3.4", ">=1.3.4 <2.0.0"},     
  };
  size_t i;
  semver_version_req_wrapped r;

  for (i = 0; i < sizeof(tests) / sizeof(exp2_t); i++) {
    char buf[SEMVER_MAXLEN];
    int k = 0;

    r = semver_version_req_from_string_wrapped(tests[i].inp);
    TEST_ASSERT_FALSE(r.err);

    TEST_ASSERT_NOT_NULL(r.unwrap.result);

    k = semver_version_req_sprint(r.unwrap.result, buf);
    TEST_ASSERT_EQUAL_STRING((const char *)&buf, tests[i].exp);
    TEST_ASSERT(k > 0);

    semver_version_req_delete(r.unwrap.result);
  }
}

extern void parse_version_req(const char *s, void *result);
typedef struct {
  char comparator_buf[4];
  int comparator_valid;
  semver_version l;
  const char *last;
  int found_comparator_parts;
  int found_semver_parts;
} extern_req_parser_result_t;

typedef struct {
  const char *inp;
  const char *exp_comp;
  const char *exp_v;
} exp3_t;

void test_wb_parse_version_req(void) {
  const exp3_t tests[] = {
      {"=1.2.0", "=", "1.2.0"},
      {" >= 1.2.3-snapshot+SHA123 ", ">=", "1.2.3-snapshot+SHA123"},
      {"0.0.1", "=", "0.0.1"},
      {"<2.0.0", "<", "2.0.0"},
      {"<=2.0.0", "<=", "2.0.0"},
      {"~2.0.0", "~", "2.0.0"}};
  size_t i;

   for (i = 0; i < sizeof(tests) / sizeof(exp3_t); i++) {
    extern_req_parser_result_t res;
    char buf[SEMVER_MAXLEN];

    parse_version_req(tests[i].inp, &res);

    TEST_ASSERT_EQUAL_STRING(res.comparator_buf, tests[i].exp_comp);
    TEST_ASSERT_NOT_NULL(res.l);

    semver_version_sprint(res.l, buf);

    TEST_ASSERT_EQUAL_STRING(buf, tests[i].exp_v);

    TEST_ASSERT_TRUE(res.comparator_valid);
  }
}

typedef struct {
  const char *inp;
  const int comparator_valid;
  const int version_valid;
} exp4_t;

void test_wb_parse_version_req_invalid(void) {
  const exp4_t tests[] = {
      {"==1.2.0", 0, 1}, {">>1.2.0", 0, 1}, {"<>1.2.0", 0, 1},
      {"<~1.2.0", 0, 1}, {"^=1.2.0", 0, 1},
  };
  size_t i;
  for (i = 0; i < sizeof(tests) / sizeof(exp4_t); i++) {
    extern_req_parser_result_t res;
    parse_version_req(tests[i].inp, &res);

    TEST_ASSERT_EQUAL(tests[i].comparator_valid, res.comparator_valid);
    TEST_ASSERT_EQUAL(tests[i].version_valid, res.l != 0);
  }
}

void test_wb_parse_version_req_2nd(void) {
  const exp3_t tests[] = {{">=1.4.3 <=1.9.0", "<=", "1.9.0"},
                          {">=1.4.3; <1.9.0", "<", "1.9.0"}};
  size_t i;
  for (i = 0; i < sizeof(tests) / sizeof(exp3_t); i++) {
    char buf[255];
    extern_req_parser_result_t res_first, res_second;
    parse_version_req(tests[i].inp, &res_first);

    TEST_ASSERT_NOT_NULL(res_first.l);
    TEST_ASSERT_NOT_NULL(res_first.last);

    /* continue parsing from where we left */
    parse_version_req(res_first.last, &res_second);

    TEST_ASSERT_EQUAL_STRING(res_second.comparator_buf, tests[i].exp_comp);
    TEST_ASSERT_NOT_NULL(res_second.l);

    semver_version_sprint(res_second.l, buf);

    TEST_ASSERT_EQUAL_STRING(buf, tests[i].exp_v);

    TEST_ASSERT_TRUE(res_second.comparator_valid);
  }
}

typedef struct {
  const char *v;
  const char *r;
  const int res;
} exp5_t;

void test_semverreq_match_exact() {
  const exp5_t tests[] = {
      { "0.0.1", 0, 1 },
      { "1.45.3-alpha", 0, 1 },
      { "1.45.3-beta+some", 0, 1 },
      { "1.45.3", 0, 1 },
      };
  const size_t n = sizeof(tests)/sizeof(exp5_t);
  int res, err;
  size_t i;
  char buf[255];

  for (i = 0; i < n; i++) {
    res = 99;

    memset(buf, 0, sizeof(buf));
    buf[0] = '=';
    strcpy(&buf[1], tests[i].v);
    err = semver_matches(tests[i].v, buf, &res);

    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_NOT_EQUAL(99, res);
    TEST_ASSERT_GREATER_THAN(0, res);
  }
}

void test_semverreq_match_range() {
  const exp5_t tests[] = {
      { "0.0.0",        ">=0.0.1 <1.0.0", 0 },
      { "0.0.1-alpha",  ">=0.0.1 <1.0.0", 0 },
      { "0.0.1",        ">=0.0.1 <1.0.0", 1 },
      { "0.0.1",        ">0.0.1 <1.0.0", 0 },
      { "0.0.2",        ">0.0.1 <1.0.0", 1 },
      { "0.1.0",        ">0.0.1 <1.0.0", 1 },
      { "0.9.9-alpha",  ">0.0.1 <1.0.0", 1 },
      { "1.0.0",        ">0.0.1 <1.0.0", 0 },
      { "1.0.0",        ">0.0.1 <=1.0.0", 1 },
      { "1.3.0",        ">=1.3.0 <2.0.0", 1 },
      { "1.45.3",       ">=1.3.0 <2.0.0", 1 },
      { "2.0.0",        ">=1.3.0 <2.0.0", 0 }
  };
  const size_t n = sizeof(tests)/sizeof(exp5_t);
  int res, err;
  size_t i;

  for (i = 0; i < n; i++) {
    res = 99;
    err = semver_matches(tests[i].v, tests[i].r, &res);

    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_NOT_EQUAL(99, res);
    TEST_ASSERT_EQUAL(tests[i].res, res);
  }
}

void test_semverreq_match_range_ops() {
  const exp5_t tests[] = {
      { "1.1.3",        "~1.1.0", 1 },
      { "1.1.3",        "~1.1.1", 1 },
      { "1.2.3",        "~1.1.1", 0 },
      { "1.1.3",        "~1.0.1", 0 },
  };
  const size_t n = sizeof(tests)/sizeof(exp5_t);
  int res, err;
  size_t i;

  for (i = 0; i < n; i++) {
    res = 99;
    err = semver_matches(tests[i].v, tests[i].r, &res);

    TEST_ASSERT_EQUAL(0, err);
    TEST_ASSERT_NOT_EQUAL(99, res);
    TEST_ASSERT_EQUAL(tests[i].res, res);
  }
}

#define vv    "1.0.0"
#define vreq  ">=0.0.0 <99.99.99"

void test_semverreq_match_invalid() {
  const exp5_t tests[] = {
      { 0, vreq, 0},
      { vv, 0, 0 },
      { "0.a.0",        vreq, 0 },
      { vv, "!~1.1.1", 0}
  };
  const size_t n = sizeof(tests)/sizeof(exp5_t);
  int res, err;
  size_t i;

  err = semver_matches(vv, vreq, 0);
  TEST_ASSERT_NOT_EQUAL(0, err);

  for (i = 0; i < n; i++) {
    res = 99;
    err = semver_matches(tests[i].v, tests[i].r, &res);

    TEST_ASSERT_NOT_EQUAL(0, err);
    TEST_ASSERT_EQUAL(99, res);
  }
}

void run_semverreq_tests(void) {
  /* explicitly constructed semverreqs should print correctly */
  RUN_TEST(test_semverreq_print);

  /* invalid req must be rejected by parser */
  RUN_TEST(test_semverreq_invalid);

  /* white box tests of the parser for valid and invalid reqs */
  RUN_TEST(test_wb_parse_version_req);
  RUN_TEST(test_wb_parse_version_req_2nd);
  RUN_TEST(test_wb_parse_version_req_invalid);

  /* parsing should work correctly */
  RUN_TEST(test_semverreq_parse);

  /* high level match function should work correctly */
  RUN_TEST(test_semverreq_match_exact);
  RUN_TEST(test_semverreq_match_range);
  RUN_TEST(test_semverreq_match_range_ops);
  RUN_TEST(test_semverreq_match_invalid);
}
