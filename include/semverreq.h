#ifndef __SEMVERREQ_H
#define __SEMVERREQ_H

#include "semver.h"

#ifdef __cplusplus
extern "C" {
#endif

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
  SEMVERREQ_EOI = 1,
  /** a semver part is invalid */
  SEMVERREQ_INVALID_SEMVER = 2,
  /** a comparator is invalid */
  SEMVERREQ_INVALID_COMPARATOR = 3

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

#ifdef __cplusplus
}
#endif

#endif
