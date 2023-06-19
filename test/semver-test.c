#include <stdlib.h>
#include <string.h>

#include "semver.h"

#include "unity.h"

/* white-box test */
extern int semver_version_from_string_impl(semver_version self, const char *s);
extern int semver_version_prerelease_cmp(const char *a, const char *b);
extern semver_version semver_version_new(void);

typedef struct {
  const char *inp;
  const int exp_rc;
} exp_t;

typedef struct {
  const char *inp;
  const int exp_maj, exp_min, exp_pat;
  const char *exp_prerelease;
  const char *exp_build;
} exp2_t;

typedef struct {
  const int inp_major, inp_minor, inp_patch;
  const char *inp_pre, *inp_build;
  const char *exp_formatted;
} exp3_t;

typedef struct {
  const char *inp_a;
  const char *inp_b;
  const int exp_res;
} exp4_t;

void test_semver_invalid_parsing(void) {
  const exp_t tests[] = {
      {"", SEMVER_ERROR_PARSE_PREMATURE_EOS},
      {"a.b.c", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"-", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"1", SEMVER_ERROR_PARSE_PREMATURE_EOS},
      {"1.", SEMVER_ERROR_PARSE_PREMATURE_EOS},
      {"1.2", SEMVER_ERROR_PARSE_PREMATURE_EOS},
      {"1.2.", SEMVER_ERROR_PARSE_PREMATURE_EOS},
      {"1.b.3", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"a.2.3", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"1.2.c", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"01.2.3", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"1.02.3", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"1.2.03", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"1.2.3-we%rd+stuff", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
      {"1.2.3-weird+st$ff", SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE},
  };

  size_t i;
  semver_version_wrapped w1;
  int res = 0;
  semver_version s1 = NULL;
  for (i = 0; i < sizeof(tests) / sizeof(exp_t); i++) {

    /* use both construction versions */
    s1 = semver_version_from_string(tests[i].inp);
    TEST_ASSERT_NULL(s1);

    w1 = semver_version_from_string_wrapped(tests[i].inp);
    TEST_ASSERT_TRUE(w1.err);
    TEST_ASSERT_EQUAL(w1.unwrap.code, tests[i].exp_rc);

    /* white box test to check result codes */
    s1 = semver_version_new();
    res = semver_version_from_string_impl(s1, tests[i].inp);
    TEST_ASSERT_EQUAL(res, tests[i].exp_rc);
    semver_version_delete(s1);
  }
}

void test_semver_valid_parsing(void) {
  const char *inp[] = {"0.0.0",
                       "0.0.1",
                       "0.1.0",
                       "1.0.0",
                       "1.1.0",
                       "1.1.1",
                       "2.3.4",
                       "45.465.374-beta.some.thing",
                       "13.45.2-alpha.1+SHA-4711",
                       "237.347.239+BUILD1"
      };

  const exp2_t tests[] = {
      {"2.3.4", 2, 3, 4, NULL, NULL},
      {"2.3.4-with.pre.rel", 2, 3, 4, "with.pre.rel", NULL},
      {"2.3.4-with.pre.rel+andbuild", 2, 3, 4, "with.pre.rel", "andbuild"},
      {"2.3.4+onlybuild", 2, 3, 4, NULL, "onlybuild"},
      {"45.465.374-beta.some.thing", 45, 465, 374, "beta.some.thing", NULL},
      {"13.45.2-alpha.1+SHA-4711", 13, 45, 2, "alpha.1", "SHA-4711"},
  };

  size_t i;
  unsigned long a, b, c;
  size_t k;
  char buf[255];
  semver_version s1;
  semver_version_wrapped w1;

  for (i = 0; i < sizeof(inp) / sizeof(const char *); i++) {
    s1 = semver_version_from_string(inp[i]);
    TEST_ASSERT_NOT_NULL(s1);
    semver_version_delete(s1);

    w1 = semver_version_from_string_wrapped(inp[i]);
    TEST_ASSERT_FALSE(w1.err);
    semver_version_delete(w1.unwrap.result);
  }

  for (i = 0; i < sizeof(tests) / sizeof(exp2_t); i++) {
    s1 = semver_version_from_string(tests[i].inp);

    TEST_ASSERT_NOT_NULL(s1);
    TEST_ASSERT_EQUAL(semver_version_get_major(s1), tests[i].exp_maj);
    TEST_ASSERT_EQUAL(semver_version_get_minor(s1), tests[i].exp_min);
    TEST_ASSERT_EQUAL(semver_version_get_patch(s1), tests[i].exp_pat);

    semver_version_get(s1, &a, &b, &c);
    TEST_ASSERT_EQUAL(a, tests[i].exp_maj);
    TEST_ASSERT_EQUAL(b, tests[i].exp_min);
    TEST_ASSERT_EQUAL(c, tests[i].exp_pat);

    k = semver_version_copy_prerelease(s1, buf, sizeof(buf));
    if (tests[i].exp_prerelease) {
      TEST_ASSERT_EQUAL_STRING(buf, tests[i].exp_prerelease);
      TEST_ASSERT_GREATER_THAN(0, k);
    } else {
      TEST_ASSERT_EQUAL(k, 0);
      TEST_ASSERT_EQUAL(buf[0], 0);
    }

    k = semver_version_copy_build(s1, buf, sizeof(buf));
    if (tests[i].exp_build) {
      TEST_ASSERT_EQUAL_STRING(buf, tests[i].exp_build);
      TEST_ASSERT_GREATER_THAN(0, k);
    } else {
      TEST_ASSERT_EQUAL(k, 0);
      TEST_ASSERT_EQUAL(buf[0], 0);
    }

    semver_version_delete(s1);
  }
}

void test_semver_formatting(void) {
  const char *inp[] = {"2.3.4", "45.465.374-beta.some.thing",
                       "13.45.2-alpha.1+SHA-4711"};

  char buf[128];
  int res;
  size_t i;
  semver_version s1;
  for (i = 0; i < sizeof(inp) / sizeof(const char *); i++) {
    s1 = semver_version_new();

    res = semver_version_from_string_impl(s1, inp[i]);
    TEST_ASSERT_EQUAL(res, SEMVER_OK);
#ifdef __HAS_SNPRINTF__
    semver_version_snprint(s1, buf, sizeof(buf));
#else
    semver_version_sprint(s1, buf);
#endif
    TEST_ASSERT_EQUAL_STRING(inp[i], buf);

    semver_version_delete(s1);
  }
}

void test_semver_constructing(void) {
  const exp3_t tests[] = {
      {1, 2, 3, NULL, NULL, "1.2.3"},
      {1, 2, 3, "", "", "1.2.3"},
      {1, 2, 3, "alpha.1", NULL, "1.2.3-alpha.1"},
      {1, 2, 3, "alpha.1", "SHA-937465", "1.2.3-alpha.1+SHA-937465"},
  };

  size_t i;
  semver_version s;
  char buf[SEMVER_MAXLEN];
  for (i = 0; i < sizeof(tests) / sizeof(exp3_t); i++) {
    s = semver_version_from(
        tests[i].inp_major, tests[i].inp_minor, tests[i].inp_patch,
        tests[i].inp_pre, tests[i].inp_build);

    memset(buf, 0, sizeof(buf));
#ifdef __HAS_SNPRINTF__
    semver_version_snprint(s, buf, sizeof(buf));
#else
    semver_version_sprint(s, buf);
#endif
    TEST_ASSERT_EQUAL_STRING(tests[i].exp_formatted, buf);

    semver_version_delete(s);
  }
}

void test_semver_cmp(void) {
  const exp4_t tests[] = {
      {"1.0.0", "2.0.0", -5},
      {"2.0.0", "1.0.0", 5},
      {"1.1.0", "1.2.0", -4},
      {"1.2.0", "1.1.0", 4},
      {"1.1.1", "1.1.2", -3},
      {"1.1.2", "1.1.1", 3},
      {"1.1.2", "1.1.1-alpha.1", 3},
      {"1.1.2-alpha.1", "1.1.1", 3},
      {"1.1.2-alpha.1", "1.1.1-alpha.1", 3},
      {"1.1.2-alpha.1", "1.1.2-alpha.2", -2},
      {"1.1.2-alpha.1", "1.1.2-alpha.1.longer", -2},
  };

  size_t i;
  semver_version a,b;
  for (i = 0; i < sizeof(tests) / sizeof(exp4_t); i++) {
    a = semver_version_from_string(tests[i].inp_a);
    b = semver_version_from_string(tests[i].inp_b);

    TEST_ASSERT_EQUAL(semver_version_cmp(a, b), tests[i].exp_res);

    /* reverse order == inverse result */
    TEST_ASSERT_EQUAL(semver_version_cmp(b, a), tests[i].exp_res * (-1));

    /* compare with self is always 0 */
    TEST_ASSERT_EQUAL(semver_version_cmp(a, a), 0);
    TEST_ASSERT_EQUAL(semver_version_cmp(b, b), 0);

    semver_version_delete(a);
    semver_version_delete(b);
  }
}

void test_semver_cmp2(void) {

  /*
   * https://semver.org/spec/v2.0.0.html 11.4.4
   * 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2
   * < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0.
   */
  const char *arr[] = {"1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta",
                       "1.0.0-beta",  "1.0.0-beta.2",  "1.0.0-beta.11",
                       "1.0.0-rc.1",  "1.0.0"};
  size_t i;
  semver_version a,b;
  int r;
  for (i = 1; i < sizeof(arr) / sizeof(const char *); i++) {
    a = semver_version_from_string(arr[i - 1]);
    b = semver_version_from_string(arr[i]);

    r = semver_version_cmp(a, b);
    TEST_ASSERT_LESS_THAN(0, r);

    semver_version_delete(a);
    semver_version_delete(b);
  }
}

/* whitebox test the prerelease compare function with specific perrelease
 * strings */
void test_semver_prerelease_cmp(void) {
  const exp4_t tests[] = {
      {"alpha", "alpha", 0},
      {"alpha.1", "alpha.1", 0},
      {"alpha.1.2", "alpha.1.2", 0},
      {"alpha.some.more.parts", "alpha.some.more.parts", 0},
      {"alpha", "beta", -1},
      {"alpha.1", "beta.1", -1},
      {"alpha.1", "alpha.2", -2},
      {"alpha.1", "alpha.1.some", -3},
      {"alpha.1.2", "alpha.1.10", -3},
      {"alpha.1.10", "alpha.1.11", -3},
      {"alpha.1.10", "alpha.1.10.some", -4},
      {"", "", 0}, /* empty strings are considered equal too */
  };

  size_t i;
  for (i = 0; i < sizeof(tests) / sizeof(exp4_t); i++) {
    TEST_ASSERT_EQUAL(
        semver_version_prerelease_cmp(tests[i].inp_a, tests[i].inp_b),
        tests[i].exp_res);
    TEST_ASSERT_EQUAL(
        semver_version_prerelease_cmp(tests[i].inp_b, tests[i].inp_a),
        tests[i].exp_res * (-1));
  }
}

void test_semver_copy(void) {
  const exp_t tests[] = {
    { "1.2.3", 0 },
  };
  size_t i;
  char buf[SEMVER_MAXLEN];
  semver_version p, q;
  TEST_ASSERT_NULL(semver_version_from_copy(0));
  
  for (i = 0; i < sizeof(tests) / sizeof(exp_t); i++) {
    p = semver_version_from_string(tests[i].inp);
    TEST_ASSERT_NOT_NULL(p);

    q = semver_version_from_copy(p);
    TEST_ASSERT_NOT_NULL(p);

    semver_version_sprint(q, buf);
    TEST_ASSERT_EQUAL_STRING(tests[i].inp, buf);

    semver_version_delete(q);
    semver_version_delete(p);
  }


}

void test_semver_cmp3(void) {

  const char *arr[] = {
      "0.0.0",
      "0.0.1", "0.0.2",
      "0.1.0", "0.1.1-alpha", "0.1.1",
      "0.9.0", "0.9.9-beta", "0.9.9",
      "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta",
      "1.0.0-beta",  "1.0.0-beta.2",  "1.0.0-beta.11",
      "1.0.0-rc.1",  "1.0.0",
      "1.0.1-alpha", "1.0.1", "1.0.2",
      "1.1.0", "1.1.1",
      "1.9.0",
      "2.0.0",
      "99.99.99"
    };

  size_t i, j;
  int comp_res, r;
  size_t n = sizeof(arr) / sizeof(const char *);
  for (i = 1; i < n; i++) {
    comp_res = 99;

    r = semver_cmp(arr[i - 1], arr[i], &comp_res);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_LESS_THAN(0, comp_res);

    r = semver_cmp(arr[i], arr[i-1], &comp_res);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_GREATER_THAN(0, comp_res);

    r = semver_cmp(arr[i-1], arr[i-1], &comp_res);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_EQUAL(0, comp_res);

  }
  for (i = 0, j=n-1; i < n/2; i++, j--) {
    comp_res = 99;

    r = semver_cmp(arr[i], arr[j], &comp_res);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_LESS_THAN(0, comp_res);

    r = semver_cmp(arr[j], arr[i], &comp_res);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_GREATER_THAN(0, comp_res);

  }
}

void test_semver_cmp3_invalid(void) {
  int comp_res, r;

  r = semver_cmp(0, 0, 0);
  TEST_ASSERT_NOT_EQUAL(0, r);

  r = semver_cmp("not-valid", 0, &comp_res);
  TEST_ASSERT_NOT_EQUAL(0, r);

  r = semver_cmp("1.2.3", "in-valid", &comp_res);
  TEST_ASSERT_NOT_EQUAL(0, r);
}

void run_semver_tests(void) {
  int i;
  for (i = 0; i < 1; i++) {
    RUN_TEST(test_semver_formatting);
    RUN_TEST(test_semver_valid_parsing);
    RUN_TEST(test_semver_invalid_parsing);
    RUN_TEST(test_semver_constructing);
    RUN_TEST(test_semver_cmp);
    RUN_TEST(test_semver_cmp2);
    RUN_TEST(test_semver_prerelease_cmp);
    RUN_TEST(test_semver_copy);
    RUN_TEST(test_semver_cmp3);
    RUN_TEST(test_semver_cmp3_invalid);
  }
}
