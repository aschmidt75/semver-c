#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semver.h"

#define SEMVER_NEW(obj, type)                                                  \
  do {                                                                         \
    obj = malloc(sizeof(type));                                                \
    if (!obj) {                                                                \
      printf("Malloc Error: %s\n", __func__);                                  \
      assert(0);                                                               \
    }                                                                          \
  } while (0)

/**
 * semver_version_impl
 *
 * implements a semantic versioning (semver) record according to semver 2.0.0
 * @see https://semver.org/spec/v2.0.0.html
 */
typedef struct {

  /* Major version number */
  unsigned long major;

  /* Minor version number */
  unsigned long minor;

  /* Patch number */
  unsigned long patch;

  /* Prerelease string (optional) */
  char *prerelease;

  /* Build string (optional) */
  char *build;

} semver_version_impl;

/**
 * semver_version_from_string parses a given string into the semver version
 * record. Returns SEMVER_OK for correctly processed semver strings, or an error
 * code. May allocated memory for prerelease and build strings, use
 * semver_version_delete. Do not call multiple times on existing parsed
 * structures, may leak memory.
 */
int semver_version_from_string_impl(semver_version *self, const char *s);

semver_version *semver_version_new() {
  semver_version_impl *res;
  semver_version_impl svdefault = {0, 0, 0, NULL, NULL};

  SEMVER_NEW(res, semver_version_impl);
  *res = svdefault;

  return (semver_version *)res;
}

semver_version *semver_version_from(unsigned long major, unsigned long minor,
                                    unsigned long patch, const char *prerelease,
                                    const char *build) {

  semver_version_impl *res;
  SEMVER_NEW(res, semver_version_impl);
  res->major = major;
  res->minor = minor;
  res->patch = patch;
  if (prerelease != NULL && strlen(prerelease) > 0) {
    res->prerelease = strdup(prerelease);
  }
  if (build != NULL && strlen(build) > 0) {
    res->build = strdup(build);
  }
  return (semver_version *)res;
}

semver_version *semver_version_from_string(const char *s) {
  semver_version_impl *res = (semver_version_impl *)semver_version_new();
  int k = semver_version_from_string_impl(res, s);
  if (k != SEMVER_OK) {
    semver_version_delete(res);
    return NULL;
  }
  return (semver_version *)res;
}

semver_version_wrapped semver_version_from_string_wrapped(const char *s) {
  semver_version_wrapped res = {0, {.result = 0}};
  semver_version_impl *impl = (semver_version_impl *)semver_version_new();
  int k = semver_version_from_string_impl(impl, s);
  if (k != SEMVER_OK) {
    semver_version_wrapped res = {1,{.code = -1}};
    semver_version_delete(impl);
    res.unwrap.code = k;
    return res;
  }
  res.unwrap.result = (semver_version *)impl;
  return res;
}

void semver_version_delete(semver_version *_self) {
  semver_version_impl *self = (semver_version_impl *)_self;
  if (self == NULL) {
    return;
  }
  if (self->prerelease != NULL) {
    free(self->prerelease);
    self->prerelease = NULL;
  }
  if (self->build != NULL) {
    free(self->build);
    self->build = NULL;
  }
  free(self);
}

unsigned long semver_version_get_major(const semver_version *_self) {
  semver_version_impl *self = (semver_version_impl *)_self;
  return self->major;
}

unsigned long semver_version_get_minor(const semver_version *_self) {
  semver_version_impl *self = (semver_version_impl *)_self;
  return self->minor;
}

unsigned long semver_version_get_patch(const semver_version *_self) {
  semver_version_impl *self = (semver_version_impl *)_self;
  return self->patch;
}

void semver_version_get(const semver_version *_self, unsigned long *major,
                        unsigned long *minor, unsigned long *patch) {
  semver_version_impl *self = (semver_version_impl *)_self;
  *major = self->major;
  *minor = self->minor;
  *patch = self->patch;
}

size_t semver_version_copy_prerelease(const semver_version *_self, char *str,
                                      size_t size) {
  semver_version_impl *self = (semver_version_impl *)_self;
  if (self->prerelease == NULL) {
    return 0;
  }
  return snprintf(str, size, "%s", self->prerelease);
}

size_t semver_version_copy_build(const semver_version *_self, char *str,
                                 size_t size) {
  semver_version_impl *self = (semver_version_impl *)_self;
  if (self->build == NULL) {
    return 0;
  }
  return snprintf(str, size, "%s", self->build);
}

int semver_version_from_string_impl(semver_version *_self, const char *s) {
  char *scratch = NULL;
  const char *p = s;
  char *ptr;
  char *w = scratch;

  semver_version_impl *self = (semver_version_impl *)_self;
  int n = strlen(s);
  if (n >= SEMVER_MAXLEN) {
    return SEMVER_ERROR_PARSE_TOO_LONG;
  }

  /* allocate scratch buffer */
  scratch = (char *)malloc(n + 1);
  if (!scratch) {
    printf("Malloc Error: %s\n", __func__);
    return 0;
  }
  memset(scratch, 0, n + 1);

  w = scratch;
  do {
    if (*p == 0) {
      free(scratch);
      return SEMVER_ERROR_PARSE_PREMATURE_EOS;
    }
    if (*p == '.') {
      p++;
      if (strlen(scratch) > 1 && *scratch == '0') {
        free(scratch); /* leading zero */
        return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
      }
      /* major in scratch, take */
      self->major = strtoul(scratch, &ptr, 10);
      break;
    }
    if (*p >= '0' && *p <= '9') {
      *w++ = *p++;
      continue;
    }
    /* non-matching char */
    free(scratch);
    return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;

  } while (1);

  w = scratch;
  memset(scratch, 0, n + 1);
  do {
    if (*p == 0) {
      free(scratch);
      return SEMVER_ERROR_PARSE_PREMATURE_EOS;
    }
    if (*p == '.') {
      p++;
      if (strlen(scratch) > 1 && *scratch == '0') {
        free(scratch); /* leading zero */
        return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
      }
      /* minor in scratch, take */
      self->minor = strtoul(scratch, &ptr, 10);
      break;
    }
    if (*p >= '0' && *p <= '9') {
      *w++ = *p++;
      continue;
    }
    free(scratch);
    return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
  } while (1);

  w = scratch;
  memset(scratch, 0, n + 1);
  do {
    if (*p == 0) {
      if (strlen(scratch) > 1 && *scratch == '0') {
        free(scratch); /* leading zero */
        return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
      }
      if (strlen(scratch) == 0) {
        /* handle e.g. "1.2.". we consumed the last '.', scratch is empty
           but this is not a valid semver. */
        free(scratch);
        return SEMVER_ERROR_PARSE_PREMATURE_EOS;
      }
      self->patch = strtoul(scratch, &ptr, 10);
      /* patch version read, minimal semver reached */
      free(scratch);
      return SEMVER_OK;
    }
    if ((*p == '-') || (*p == '+')) {
      if (strlen(scratch) > 1 && *scratch == '0') {
        free(scratch); /* leading zero */
        return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
      }
      /* patch read, more to follow */
      self->patch = atoi(scratch);
      break;
    }
    if (*p >= '0' && *p <= '9') {
      *w++ = *p++;
      continue;
    }
    free(scratch);
    return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
  } while (1);

  if (*p == '-') {
    p++;

    /* prerelease to follow .. */

    w = scratch;
    memset(scratch, 0, n + 1);
    do {
      if (*p == 0) {
        self->prerelease = strdup(scratch);
        free(scratch);
        return SEMVER_OK;
      }
      if (*p == '+') {
        self->prerelease = strdup(scratch);
        /* prerelease read, more to follow */
        break;
      }
      if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') ||
          (*p >= 'A' && *p <= 'Z') || (*p == '-') || (*p == '.')) {
        *w++ = *p++;
        continue;
      }
      free(scratch);
      return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
    } while (1);
  }

  if (*p == '+') {
    p++;

    /* build strign to follow here */
    w = scratch;
    memset(scratch, 0, n + 1);
    do {
      if (*p == 0) {
        self->build = strdup(scratch);
        free(scratch);
        return SEMVER_OK;
      }
      if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') ||
          (*p >= 'A' && *p <= 'Z') || (*p == '-') || (*p == '.')) {
        *w++ = *p++;
        continue;
      }
      free(scratch);
      return SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE;
    } while (1);
  }

  free(scratch);
  return SEMVER_ERROR_STRUCTURE;
}

size_t semver_version_snprint(const semver_version *_self, char *str,
                              size_t size) {
  const semver_version_impl *self = (semver_version_impl *)_self;
  if (str == 0 || size == 0) {
    return 0;
  }
  if (self->prerelease == NULL && self->build == NULL) {
    return snprintf(str, size, "%lu.%lu.%lu", self->major, self->minor,
                    self->patch);
  }
  if (self->prerelease != NULL && self->build == NULL) {
    return snprintf(str, size, "%lu.%lu.%lu-%s", self->major, self->minor,
                    self->patch, self->prerelease);
  }
  if (self->prerelease == NULL && self->build != NULL) {
    return snprintf(str, size, "%lu.%lu.%lu+%s", self->major, self->minor,
                    self->patch, self->build);
  }
  if (self->prerelease != NULL && self->build != NULL) {
    return snprintf(str, size, "%lu.%lu.%lu-%s+%s", self->major, self->minor,
                    self->patch, self->prerelease, self->build);
  }
  return -1;
}

int count_dots(const char *s) {
  int na = 0;
  if (s) {
    while (*s++ != 0) {
      na += (*s == '.') ? 1 : 0;
    }
  }
  return na;
}

int atoi_checked(const char *s, unsigned long *res) {
  const char *p = s;
  int i = 0;
  char *ptr;
  
  if (s == NULL) {
    return 0;
  }

  while (*p != 0) {
    if (*p >= '0' && *p <= '9' && i <= 6) {
      /* ok */
    } else {
      /* not non-digit or too long */
      return 0;
    }
    p++;
    i++;
  }

  *res = strtoul(s, &ptr, 10);

  return 1;
}

/**
 * semver_version_prerelease_cmp compares two prerelease strings
 * precedence rules:
 * - if both are NULL or empty, no need to compare
 * - if one of the two is empty, there non-empty one has precedence
 * - compare each dot-separated part, either numerically if both are numerical,
 * or lexically. returns
 * - <0 if a < b
 * - >0 if a > b
 * - 0 if both are equal
 */
int semver_version_prerelease_cmp(const char *a, const char *b) {
  int res = 0;
  int i = 0;
  char *p;
  int na = 0;
  char *_a = NULL;
  char **arr_a = NULL;
  int nb = 0;
  char *_b = NULL;
  char **arr_b = NULL;

  /* simple checks */
  if (a == NULL && b == NULL) {
    return 0;
  }
  if (a != NULL && strlen(a) == 0 && b != NULL && strlen(b) == 0) {
    return 0;
  }
  if (a == NULL || (a != NULL && strlen(a) == 0)) {
    return 1;
  }
  if (b == NULL || (b != NULL && strlen(b) == 0)) {
    return -1;
  }

  /* split each string by '.' */
  na = count_dots(a) + 1;
  _a = strdup(a);

  arr_a = (char **)calloc(na, sizeof(char *));
  p = _a;
  i = 0;
  arr_a[i] = p;
  while (*p != 0) {
    if (*p == '.') {
      *p = 0; /* terminate this part */
      i++;
      p++;
      arr_a[i] = p;
    }
    p++;
  }

  /* -- b
    split each string by '.' */
  nb = count_dots(b) + 1;
  _b = strdup(b);

  arr_b = (char **)calloc(nb, sizeof(char *));
  p = _b;
  i = 0;
  arr_b[i] = p;
  while (*p != 0) {
    if (*p == '.') {
      *p = 0;
      i++;
      p++;
      arr_b[i] = p;
    }
    p++;
  }

  /* from here we can compare arr_a and arr_b, part-wise */
  i = 0;
  while (i < na && i < nb) {
    unsigned long av = 0, bv = 0;
    int k = 0;

    if (i >= na) {
      break;
    }
    if (i >= nb) {
      break;
    }
    /* if both parts are numbers, compare numerically; lexically otherwise */
    k = 0;
    if (atoi_checked(arr_a[i], &av) && atoi_checked(arr_b[i], &bv)) {
      if (av < bv) {
        k = -1;
      }
      if (av > bv) {
        k = 1;
      }
    } else {
      k = strcmp(arr_a[i], arr_b[i]);
    }
    if (k < 0) {
      res = (i + 1) * (-1);
      goto cleanup;
    }
    if (k > 0) {
      res = (i + 1);
      goto cleanup;
    }
    /* == 0, both parts are equal, cont. */

    i++;
  }
  /* if we got here, both arr's are considered equal up to this point
     if any of the two is LONGER (i.e. has still elements to come), then the
     OTHER one has precendence */
  res = 0;
  if (na > nb) {
    res = (i + 1);
    goto cleanup;
  }
  if (nb > na) {
    res = (i + 1) * (-1);
  }

cleanup:
  free(arr_b);
  free(_b);
  free(arr_a);
  free(_a);

  return res;
}

/**
 * internal: return codes indicate the spot where a and b differ
 * 5/-5: major version
 * 4/-4: minor version
 * 3/-3: patch level
 * 2/-2: prerelease
 * 0: equals
 */
int semver_version_cmp(const semver_version *_a, const semver_version *_b) {
  int p;
  const semver_version_impl *a = (semver_version_impl *)_a;
  const semver_version_impl *b = (semver_version_impl *)_b;
  if (a->major < b->major) {
    return -5;
  }
  if (a->major > b->major) {
    return 5;
  }
  /* major's equal, check minor */
  if (a->minor < b->minor) {
    return -4;
  }
  if (a->minor > b->minor) {
    return 4;
  }
  /* minor equals too, check patch */
  if (a->patch < b->patch) {
    return -3;
  }
  if (a->patch > b->patch) {
    return 3;
  }
  /* patch equals too, compare prereleases */
  p = semver_version_prerelease_cmp(a->prerelease, b->prerelease);
  if (p < 0) {
    return -2;
  }
  if (p > 0) {
    return 2;
  }
  return 0;
}
