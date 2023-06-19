/*
 * MIT License
 *
 * Copyright 2023 @aschmidt75
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "semver.h"
#include "semverreq.h"

int semver_matches(const char *version_str, const char *versionreq_str, int *res) {
  semver_version_req r = 0;
  semver_version v = 0;
  int err = 1;

  if (!res || !version_str || !versionreq_str ) {
    goto end_delete;
  }

  v = semver_version_from_string(version_str);
  if (!v) {
    goto end_delete;
  }

  r = semver_version_req_from_string(versionreq_str);
  if (!r) {
    goto end_delete;
  }

  *res = semver_version_req_matches(r, v);
  err = 0;

end_delete:
  if(v) {
    semver_version_delete(v);
  }
  if(r) {
    semver_version_req_delete(r);
  }
  return err;
}

#define SEMVERREQ_NEW(obj, type)                                               \
  do {                                                                         \
    obj = malloc(sizeof(type));                                                \
    if (!obj) {                                                                \
      printf("Malloc Error\n");                                                \
      assert(0);                                                               \
    }                                                                          \
  } while (0);


/**
 * The implementation of a semver requirement consist of a lower
 * and an upper bound. Flags indicate if the bound is including (e.g. >=)
 * or not (>). For an exact version match, lower == upper and both are
 * including. For an open-end requirement (e.g. >1.0.0), only one of
 * the bounds is set. Lower must always be <= upper.
 * Both lower and upper == null is an invalid requirement.
 * An all-matching requirement can be expressed by ">=0.0.0" and upper = null.
 * This structure does not support tilde and caret operators, these are
 * converted through the parsing process before.
 */
struct semver_version_req_impl {
  semver_version lower;
  int lower_including;

  semver_version upper;
  int upper_including;

};
typedef struct semver_version_req_impl *semver_version_req_impl;

const char *semverreq_valid_comparators[] = {
    "=", "<", ">", "<=", ">=", "^", "~"};

int semverreq_valid_comparator(const char *p) {
  if (p && *p) {
    size_t i = 0;
    for (i = 0;
         i < sizeof(semverreq_valid_comparators) / sizeof(const char *); i++) {
      if (strcmp(semverreq_valid_comparators[i], p) == 0) {
        return 1;
      }
    }
  }
  return 0;
}

int semverreq_comparator_is_including(const char *p) {
  if (p && *p) {
    if (strcmp(p, "<=") == 0 || strcmp(p, ">=") == 0 || strcmp(p, "=") == 0 ||
        strcmp(p, "~") == 0 || strcmp(p, "^") == 0) {
      return 1;
    }
  }
  return 0;
}

semver_version_req semver_version_req_from(semver_version lower_bound,
                                            int lower_including,
                                            semver_version upper_bound,
                                            int upper_including) {

  semver_version_req_impl res = 0;

  if (lower_bound && upper_bound) {
    /* pre-check: if upper < lower, return null */
    int c = semver_version_cmp(lower_bound, upper_bound);
    if (c > 0) {
      return 0;
    }

    /* if both versions are equal, then at least one of them must be including.
     * otherwise we get something as >1.0.0 <1.0.0 */
    if (c == 0) {
      if (!(lower_including != 0 || upper_including != 0)) {
        return 0;
      }
    }
  }

  SEMVERREQ_NEW(res, struct semver_version_req_impl);
  res->lower = 0;
  res->upper = 0;

  /* deep-copy */
  res->lower = (semver_version)semver_version_from_copy(lower_bound);
  res->lower_including = lower_including;

  res->upper = (semver_version)semver_version_from_copy(upper_bound);
  res->upper_including = upper_including;

  return (semver_version_req )res;
}

void semver_version_req_delete(semver_version_req _self) {
  semver_version_req_impl self = (semver_version_req )_self;
  if(self->lower) {
    semver_version_delete(self->lower);
  }
  if(self->upper) {
    semver_version_delete(self->upper);
  }
  free(self);
}

#define _REQ_PARSER_MAX_COMP_SIZE 4
typedef struct {
  char comparator_buf[_REQ_PARSER_MAX_COMP_SIZE];
  int comparator_valid;
  semver_version l;
  const char *last;
  int found_comparator_parts;
  int found_semver_parts;
} _req_parser_result_t;

#define SEMVER_VALID_CHAR(c)                                                   \
  ((((c) >= '0') && ((c) <= '9')) || (((c) >= 'a') && ((c) <= 'z')) ||         \
   (((c) >= 'A') && ((c) <= 'Z')) || ((c) == '.') || ((c) == '+') ||           \
   ((c) == '-'))
#define SEMVER_VALID_FIRST_CHAR(c) (((c) >= '0') && ((c) <= '9'))

void parse_version_req(const char *s, _req_parser_result_t *res) {
  char *c = 0;
  size_t cc = 0;
  char scratch[SEMVER_MAXLEN];
  char *scratchp = &scratch[0];
  semver_version_wrapped w;

  res->l = 0;
  res->last = 0;
  res->found_comparator_parts = 0;
  res->found_semver_parts = 0;
  memset(res->comparator_buf, 0, sizeof(res->comparator_buf));
  c = &res->comparator_buf[0];

  memset(scratch, 0, sizeof(scratch));

  while (s && *s) {
    res->last = s;
    if ((*s == ' ') || (*s == '\t')) {
      /* skip ws */
      s++;
      continue;
    };
    if ((*s == ',') || (*s == ';')) {
      /* skip ws */
      s++;
      continue;
    };
    if ((*s == '<') || (*s == '>') || (*s == '=') || (*s == '^') ||
        (*s == '~')) {
      /* take */
      if (++cc < _REQ_PARSER_MAX_COMP_SIZE) {
        *c++ = *s++;
      }
      res->found_comparator_parts = 1;
      continue;
    };
    if (SEMVER_VALID_FIRST_CHAR(*s)) {

      /* semver_version starts here, take everything from it into buffer */
      while (s && *s && SEMVER_VALID_CHAR(*s)) {
        *scratchp++ = *s++;
      };
      res->last = s;
      res->found_semver_parts = 1;
      *scratchp = '\0';

      res->l = 0;
      /* try to parse what we have up until now */
      w = semver_version_from_string_wrapped(scratch);
      if (w.err) {
        /* some error occured */
        break;
      } else {
        res->l = w.unwrap.result;
      }

      break;
    };
    /* if we get here, an invalid char occured. */
    break;
  }
  /* if comparator_buf is empty, no comparator has been given. Use = as default */
  if (res->comparator_buf[0] == '\0') {
    strcpy(res->comparator_buf, "=");
  }
  res->comparator_valid = semverreq_valid_comparator(res->comparator_buf);
}

semver_version_req semver_version_req_from_string(const char *str) {
  semver_version_req_wrapped res;

  if (str == 0 || strlen(str) == 0) {
    return 0;
  }

  res = semver_version_req_from_string_wrapped(str);
  if (res.err) {
    return 0;
  }
  return res.unwrap.result;
}

semver_version_req_wrapped
semver_version_req_from_string_wrapped(const char *str) {

  char *p = 0;
  int st = 0;
  semver_version_req_impl res = 0;
  _req_parser_result_t part1, part2;
  semver_version_req_wrapped w;
  int swap_flags[2] = { 0, 0 };
  int special_op = 0;
  unsigned long ma = 0;
  unsigned long mi = 0;
  unsigned long pa = 0;
  w.err = 0;

  memset(part1.comparator_buf,0,sizeof(part1.comparator_buf));
  part1.last = 0; part1.l = 0;
  memset(part2.comparator_buf,0,sizeof(part2.comparator_buf));
  part2.last = 0; part2.l = 0;

  if (str == 0 || strlen(str) == 0) {
    semver_version_req_wrapped err;
    err.err = 1;
    err.unwrap.code = SEMVERREQ_EOI;
    return err;
  }

  res = (semver_version_req_impl )malloc(sizeof(struct semver_version_req_impl));
  res->upper = 0;
  res->lower = 0;

  parse_version_req(str, &part1);

  if (part1.l == 0) {
    /* first part not parsed successful */
    semver_version_req_wrapped err;
    err.err = 1;
    err.unwrap.code = SEMVERREQ_INVALID_SEMVER;
    free(res);
    return err;
  }
  if (!part1.comparator_valid) {
    semver_version_req_wrapped err;
    err.err = 1;
    err.unwrap.code = SEMVERREQ_INVALID_COMPARATOR;
    free(res);
    return err;
  }

  /* TODO check for 1 or 2 parts, apply rules */
  /* do we have chars left other than whitespaces? */
  p = (char*)part1.last;
  while (p && *p) {
    if( *p == ' ' || *p == '\t') {
      /* ws */
    } else {
      /* something else left to parse */
      st = 1;
      break;
    }
    p++;
  }

  res->lower = part1.l;
  res->lower_including =
      semverreq_comparator_is_including(part1.comparator_buf);

  /* handle case exact match (e.g. =1.0.0). Then set upper bound accordingly
   * internal repr is >=1.0.0 <=1.0.0, which sprintf outputs as "=1.0.0"
   * As this is an exact version req, we can stop parsing here.
   */
  if(strcmp(part1.comparator_buf, "=") == 0) {
    res->upper = semver_version_from_copy(res->lower);
    res->upper_including = 1;
    goto fin;
  }

  if (!st) {
    /* rest of the input was empty or whitespaces, so we have only the first part */
    res->upper = 0;

    /* treat caret and tilde operator here */
    if (strcmp(part1.comparator_buf, "~") == 0) {
      /*
      tilde == flexible patch
      set upper part, e.g.
      ~1.3.5 to < 1.4.0
      */
      res->upper_including = 0;
      ma = semver_version_get_major(part1.l);
      mi = semver_version_get_minor(part1.l);

      res->upper = semver_version_from(ma, mi+1, 0, 0, 0);
      special_op = 1;
    }
    if (strcmp(part1.comparator_buf, "^") == 0) {
      ma = semver_version_get_major(part1.l);
      mi = semver_version_get_minor(part1.l);

      if (ma == 0 && mi == 0) {
        pa = semver_version_get_patch(part1.l);

        /* no flexibility: only the exact version will match */
        res->upper_including = res->lower_including = 1;

        res->upper = semver_version_from(0, 0, pa, 0, 0);
        special_op = 1;
      } else {
        if (ma == 0 && mi >= 1) {
          /* major zero, increase minor */
          res->upper_including = 0;
          res->upper = semver_version_from(ma, mi+1, 0, 0, 0);
          special_op = 1;
        } else {
          /*
           Caret == flexible minor + patch
           set upper part, e.g.
           ~1.3.5 to < 2.0.0
           ~3.3 to < 4.0.0
           */
          res->upper_including = 0;
          res->upper = semver_version_from(ma+1, 0, 0, 0, 0);
          special_op = 1;

        }
      }

    }
  } else {
    /** Only look at the rest if no special operator (tilde, caret) has been processed */
    if (special_op == 0) {
      parse_version_req(part1.last, &part2);

      if (part2.l == 0) {
        semver_version_req_wrapped err;
        err.err = 1;
        err.unwrap.code = SEMVERREQ_INVALID_SEMVER;

        /* 2nd part not parsed successful, check why */
        free(res);
        free(part1.l);
        return err;
      }
      if (!part2.comparator_valid) {
        semver_version_req_wrapped err;
        err.err = 1;
        err.unwrap.code = SEMVERREQ_INVALID_COMPARATOR;

        free(res);
        free(part1.l);
        return err;
      }

      /* for now assume part1 == lower, part== upper. */
      res->upper = part2.l;
      res->upper_including =
          semverreq_comparator_is_including(part2.comparator_buf);
    }

  }

  /*
   * check if upper is actually a lower bound (>, >=) AND/OR
   * lower is actually an upper bound (<, <=). If so, swap the two
   * so that lower is always <= upper
   */
  if ( res->upper && res->lower) {
    if (res->upper) {
      if (
          strcmp(part2.comparator_buf, ">") == 0 ||
          strcmp(part2.comparator_buf, ">=") == 0
          ) {
        swap_flags[1] = 1;
      }
    }
    if (res->lower) {
      if (
          strcmp(part1.comparator_buf, "<") == 0 ||
          strcmp(part1.comparator_buf, "<=") == 0
          ) {
        swap_flags[0] = 1;
      }
    }
    if (swap_flags[0]+swap_flags[1] > 0) {
      semver_version swap_version = res->upper;
      int swap_inc = res->upper_including;

      res->upper = res->lower;
      res->upper_including = res->lower_including;

      res->lower = swap_version;
      res->lower_including = swap_inc;
    }
  } else {
    /** if we have only a lower bound, check if this is actually an upper bound */
    if ( !res->upper && res->lower) {
      if (
          strcmp(part1.comparator_buf, "<") == 0 ||
          strcmp(part1.comparator_buf, "<=") == 0
          ) {
        res->upper = res->lower;
        res->upper_including = res->lower_including;

        res->lower = 0;
        res->lower_including = 0;
      }
    }

  }

fin:
  w.unwrap.result = res;

  return w;
}

#ifdef __HAS_SNPRINTF__
int semver_version_req_snprint(semver_version_req _self, char *buf,
                               size_t sz) {
  semver_version_req_impl self = (semver_version_req_impl )_self;
  char lower_cmp[8] = "\0\0\0";
  char upper_cmp[8] = "\0\0\0";
  char buf1[SEMVER_MAXLEN];
  char buf2[SEMVER_MAXLEN];
  int c;
  size_t k;

  if (buf == 0 || sz == 0) {
    return 0;
  }
  if (self->lower_including == 0) {
    strcpy(lower_cmp, ">");
  } else {
    strcpy(lower_cmp, ">=");
  }
  if (self->upper_including == 0) {
    strcpy(upper_cmp, "<");
  } else {
    strcpy(upper_cmp, "<=");
  }

  if (self->lower == NULL && self->upper == NULL) {
    *buf = 0; /* no info yields empty string */
    return 0;
  }
  if (self->lower != NULL && self->upper == NULL) {
    k = semver_version_snprint(self->lower, buf1, sizeof(buf1));
    return snprintf(buf, sz, "%s%s", lower_cmp, buf1);
  }
  if (self->upper != NULL && self->lower == NULL) {
    semver_version_snprint(self->upper, buf1, sizeof(buf1));
    return snprintf(buf, sz, "%s%s", upper_cmp, buf1);
  }


  /* now if *lower == *upper, then we can simplify the output */
  c = semver_version_cmp(self->lower, self->upper);
  if (c == 0 && self->lower_including == 1 && self->upper_including == 1) {
    semver_version_snprint(self->lower, buf1, sizeof(buf1));
    return snprintf(buf, sz, "=%s", buf1);
  }

  semver_version_snprint(self->lower, buf1, sizeof(buf1));
  semver_version_snprint(self->upper, buf2, sizeof(buf2));

  return snprintf(buf, sz, "%s%s %s%s", lower_cmp, buf1, upper_cmp, buf2);
}

#endif

int semver_version_req_sprint(semver_version_req _self, char *buf) {
  semver_version_req_impl self = (semver_version_req_impl )_self;
  char lower_cmp[8] = "\0\0\0";
  char upper_cmp[8] = "\0\0\0";
  char buf1[SEMVER_MAXLEN] = "\0";
  char buf2[SEMVER_MAXLEN] = "\0";
  int c;
  size_t k;

  if (buf == 0 || self == 0) {
    return 0;
  }
  if (self->lower_including == 0) {
    strcpy(lower_cmp, ">");
  } else {
    strcpy(lower_cmp, ">=");
  }
  if (self->upper_including == 0) {
    strcpy(upper_cmp, "<");
  } else {
    strcpy(upper_cmp, "<=");
  }


  if (self->lower == 0 && self->upper == 0) {
    buf[0] = 0; /* no info yields empty string */
    return 0;
  }
  if (self->lower != 0 && self->upper == 0) {
    semver_version_sprint(self->lower, buf1);
    return sprintf(buf, "%s%s", lower_cmp, buf1);
  }
  if (self->upper != 0 && self->lower == 0) {
    semver_version_sprint(self->upper, buf1);
    return sprintf(buf, "%s%s", upper_cmp, buf1);
  }

  semver_version_sprint(self->lower, buf1);
  semver_version_sprint(self->upper, buf2);

  /* now if *lower == *upper, then we can simplify the output */
  c = semver_version_cmp(self->lower, self->upper);

  if (c == 0 && self->lower_including == 1 && self->upper_including == 1) {
    return sprintf(buf, "=%s", buf1);
  }

  return sprintf(buf, "%s%s %s%s", lower_cmp, buf1, upper_cmp, buf2);
}

int semver_version_req_matches(semver_version_req _self, semver_version v) {
  int cl, cu;

  semver_version_req_impl self = (semver_version_req_impl)_self;

  cl = semver_version_cmp(v, self->lower);
  if (cl < 0 || (cl == 0 && !self->lower_including)) {
    return 0;
    /* v is not compatible with lower bound */
  }

  cu = semver_version_cmp(v, self->upper);
  if (cu > 0 || (cu == 0 && !self->upper_including)) {
    return 0;
    /* v is not compatible with upper bound */
  }

  /* v is compatible with both lower and upper bound */
  return 1;
}
