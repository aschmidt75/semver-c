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
#ifndef __SEMVER_H
#define __SEMVER_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* use snprintf if we have it, otherwise sprintf */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define __HAS_SNPRINTF__
#endif


/**
 * semver_cmp compares two versios, a, b semver version strings != NULL
 * @param[in] a
 * @param[in] b
 * @param[out] res
 * comparison result as int only if a,b are valid semver strings
 * <0 if a < b
 *  0 if a == b
 * >0 if a > b
 * @return success of comparison operation: 0 = successful, !=0 parsing error or comparison error
 */
int semver_cmp(const char *a, const char *b, int *res);

/**
 * semver_version
 *
 * implements a semantic versioning (semver) record according to semver 2.0.0
 * see also <https://semver.org/spec/v2.0.0.html>
 */
struct semver_version;
typedef struct semver_version *semver_version;

/** semver_version_codes provides parsing error codes */
typedef enum {
  /** successful parsing */
  SEMVER_OK = 0,
  /** string larger than SEMVER_MAXLEN, unable to parse */
  SEMVER_ERROR_PARSE_TOO_LONG = 10,
  /** found premature end of string, unable to fully parse a semver */
  SEMVER_ERROR_PARSE_PREMATURE_EOS = 11,
  /** found a character that is not allowed at current position. See spec */
  SEMVER_ERROR_PARSE_NOT_ALLOWED_HERE = 12,
  /** found a structural error */
  SEMVER_ERROR_STRUCTURE = 13
} semver_version_codes;

/** semver_version_wrapped is a wrapped return value for struct creation
 * or_error */
typedef struct {
  int err;
  union {
    semver_version result;
    semver_version_codes code;
  } unwrap;
} semver_version_wrapped;

/** maximum length of a parseable semver string */
#define SEMVER_MAXLEN 255

/**
 * semver_version_new creates a new semver_version struct, initialized from
 * arguments strings can be null or are duplicated otherwise. empty strings are
 * discarded and treated as NULL. Must use semver_version_delete to free memory
 * after use.
 * @param[in] major
 * @param[in] minor
 * @param[in] patch
 * @param[in] prerelase optional prerelease string, NULL otherwise
 * @param[in] build optional build string, NULL otherwise
 * @return pointer to allocated server_version struct
 */
semver_version semver_version_from(unsigned long major, unsigned long minor,
                                    unsigned long patch, const char *prerelease,
                                    const char *build);

/**
 * semver_version_new_from_string allocates  a new semver_version struct and
 * parses from a given semver string. Returns NULL in case the given string
 * could not be parsed correctly. For error handling, see wrapped version below.
 * Must use semver_version_delete to free space.
 * @param[in] s version input string
 * @return pointer to allocated server_version struct
 */
semver_version semver_version_from_string(const char *s);

/**
 * semver_version_new_from_string_wrapped is identical to the variant above but
 * returns a wrapped response including error codes.
 * Must use semver_version_delete to free space.
 * @param[in] s version input string
 * @return wrapped result struct, including error handling
 */
semver_version_wrapped semver_version_from_string_wrapped(const char *s);

/**
 * semver_version_from_copy allocates a new semver_version struct and copies
 * the contents from given struct.
 * @param[in] v semver_version object
 * @return pointer to allocated server_version struct. Returns 0 if 0 is given.
 */
semver_version semver_version_from_copy(const semver_version v);

/**
 * semver_version_delete deletes optionally allocated memory, and delete self
 * @param[in] self semver_version struct to delete
 */
void semver_version_delete(semver_version self);

/**
 * semver_version_get_* retrieves a field from the data set, see SEMVER_FIELD_*
 */
unsigned long semver_version_get_major(const semver_version self);
unsigned long semver_version_get_minor(const semver_version self);
unsigned long semver_version_get_patch(const semver_version self);

/**
 * semver_version_get copies mahor, minor, patch version into variables
 * @param[in] self pointer to semver_version
 * @param[in] major
 * @param[in] minor
 * @param[in] patch
 */
void semver_version_get(const semver_version self, unsigned long *major,
                        unsigned long *minor, unsigned long *patch);

/**
 * semver_version_copy_prerelease copies the prerelease field to given buffer
 * with max size. Returns length of string or 0 if no prerelease information
 * exists.
 * @param[in] self pointer to semver_version
 * @param[in] str target buffer
 * @param[in] size maximum size of target buffer
 */
size_t semver_version_copy_prerelease(const semver_version self, char *str,
                                      size_t size);

/**
 * semver_version_copy_prelease copies the prerelease field to given buffer with
 * max size. Returns length of string or 0 if no prerelease information exists.
 * @param[in] self pointer to semver_version
 * @param[in] str target buffer
 * @param[in] size maximum size of target buffer
 */
size_t semver_version_copy_build(const semver_version self, char *str,
                                 size_t size);

#ifdef __HAS_SNPRINTF__
/**
 * semver_version_snprint formats the version data into a string
 * @param[in] self pointer to semver_version
 * @param[in] str target buffer
 * @param[in] size maximum size of target buffer
 * @return number of bytes copied into str
 */
size_t semver_version_snprint(const semver_version self, char *str,
                              size_t size);
#endif

/**
 * semver_version_sprint formats the version data into a string.
 * Callers must make sure to supply a buffer large enough to hold the
 * full semver string representation
 * @param[in] self pointer to semver_version
 * @param[in] str target buffer
 * @return number of bytes copied into str
 */
size_t semver_version_sprint(const semver_version self, char *str);

/**
 * semver_version_cmp compares two semver_versions, a, b != NULL
 * @param[in] a
 * @param[in] b
 * @return comparison result as int
 * <0 if a < b
 *  0 if a == b
 * >0 if a > b
 */
int semver_version_cmp(const semver_version a, const semver_version b);

#ifdef __cplusplus
}
#endif

#endif
