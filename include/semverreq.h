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
#ifndef __SEMVERREQ_H
#define __SEMVERREQ_H

#include "semver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * semver_matches checks if given semantic versioning string `version_str`
 * matches the requirements given by `versionreq_str`. This is the convenience
 * function operating on strings. Both input strings must be valid semver version
 * and requirements strings.
 * @param[in] version_str e.g.: "3.24.2"
 * @param[in] versionreq_str, e.g. ">=3.20.0 <4.0.0" or "~3.24.0"
 * @param[out] res: 1 if version matches requirements, 0 otherwise;
 * @return err: 0/SEMVERREQ_OK = successful operation, != 0 indicates an error in input parameters
 */
int semver_matches(const char *version_str, const char *versionreq_str, int *res);

/**
 * semver_version_req
 *
 * implements a semantic versioning (semver) requirement
 */
struct semver_version_req;
typedef struct semver_version_req *semver_version_req;

/** semver_version_codes provides parsing error codes */
typedef enum {
  /** successful parsing */
  SEMVERREQ_OK = 0,
  /** end of input before successful parsings */
  SEMVERREQ_EOI = 21,
  /** a semver part is invalid */
  SEMVERREQ_INVALID_SEMVER = 22,
  /** a comparator is invalid */
  SEMVERREQ_INVALID_COMPARATOR = 23

} semver_version_req_codes;

/** semver_version_req_wrapped is a wrapped return value for struct creation
 *  or error */
typedef struct {
  int err;
  union {
    semver_version_req result;
    semver_version_req_codes code;
  } unwrap;
} semver_version_req_wrapped;

/* maximum length of a parseable semver_req string */
#define SEMVERREQ_MAXLEN 512

/**
 * semver_version_req_from creates a new semver version requirement from a given
 * lower and upper bound. lower_bound or upper_bound may be 0 to indicate
 * unboundedness in either direction. the _including flags show if the bound is
 * include in the range (">=", "<=") or not (">", "<") For specifying a fixed
 * version ("="), set *lower_bound == *upper_bound Allocates memory, this must
 * be deallocated using semver_version_req_delete.
 * lower_bound and upper_bound are copied. Callers must use semver_version_req_delete
 * to deallocate this semver_version_req.
 */
semver_version_req semver_version_req_from(semver_version lower_bound,
                                            int lower_including,
                                            semver_version upper_bound,
                                            int upper_including);

/**
 * semver_version_req_from_string constructs a version requirement by parsing
 * bounds and comparisons from a string, e.g. ">=1.0.5 <2.0.0" Rules:
 * - string may contain one or two valid requirement parts (e.g. example above).
 * - string may contain leading or trailing whitespaces, comma or semicolon
 * (i.e. to separate the parts). These are ignored.
 * - in case of one part only, the other part is either unbounded (e.g.
 * ">1.0.0") or an exact version match ("=1.0.0") or a caret/tilde comparator
 * (e.g. "~3.4.2") Allocates memory, this must be deallocated using
 * semver_version_req_delete.
 */
semver_version_req semver_version_req_from_string(const char *str);

/**
 * same as above, but as wrapped version with error code handling
 */
semver_version_req_wrapped semver_version_req_from_string_wrapped(const char *str);

/**
 * Deallocates memory of a semver_version_req
 */
void semver_version_req_delete(semver_version_req self);

#ifdef __HAS_SNPRINTF__
/**
 * Formats the semver requirement into a string
 */
int semver_version_req_snprint(semver_version_req self, char *buf, size_t sz);
#endif

/**
 * Formats the semver requirement into a string
 */
int semver_version_req_sprint(semver_version_req self, char *buf);

/**
 * semver_version_req_matches checks, if `v` is within the bounds of `self`.
 * self and v must be correctly set up, otherwise the result may be wrong.
 * @returns 1 if it matches, 0 otherwise
 */
int semver_version_req_matches(semver_version_req self, semver_version v);

#ifdef __cplusplus
}
#endif

#endif
