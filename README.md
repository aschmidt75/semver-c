# semver-c
Semantic Versioning 2.0.0 in ANSI-C.

For building and testing here, [meson](https://mesonbuild.com/index.html) is required.

## Features

* compiles with c89
* no dependencies for core library parts
* supports [semver 2.0.0](https://semver.org/spec/v2.0.0.html)
* support for TILDE and CARET comparisons (see also)[https://nodesource.com/blog/semver-tilde-and-caret/], e.g.
  * `~1.3.1` includes `1.3.4` up to (not including) `1.4.0` (flexible patch)
  * `^1.1.0` includes `1.3.4` up to (not including) `2.0.0` (Caret: Flexible Minor and Patch)
  * `^0.1.0` includes `0.1.4` up to but not including `0.2.0` (Caret: Major Zero)
  * `^0.0.4` matches only `0.0.4` (Caret: Major Zero and Minor Zero)

## Usage examples

The library is divided into `semver.h` for working with semantic version strings (e.g. "1.2.0"),
and `semverreq.h` for working with requirements (e.g. ">=1.0.0 <2.0.0"). Both have convenience functions
operating on strings, as well as functions for parsing, printing and comparing into/of specialised structs.

### Convenience functions

*Compare two semver strings*

```c
#include "semver.h"
#include <assert.h>

/* ... */
  int res, err;
  err = semver_cmp( "1.0.4", "1.1.0", &res);
  assert(err == SEMVER_OK);
  assert(res < 0);
  ```

*Check if a semver version string matches a semver requirement*

```c
#include "semverreq.h"
#include <assert.h>

  /* ... */
  int res, err;
  err = semver_matches( "1.5.4", ">=1.4.0 <2.0.0", &res);
  assert(err == SEMVERREQ_OK);
  assert(res == 1);
```

### Parsing

## Test

First time setup for cloning Unity as a submodule and initializing the meson build system:

```bash
$ git submodule update --init
$ meson setup build
```

To run a test:

```bash
$ meson test -C build --print-errorlogs -v
```

