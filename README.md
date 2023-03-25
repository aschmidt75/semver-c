# semver-c
Semantic Versioning 2.0.0 in ANSI-C.

For building and testing here, [meson](https://mesonbuild.com/index.html) is required.

## Test

First time setup for pulling Unity as a submodule and initializing the meson build system:

```bash
$ git submodule update --init
$ meson setup build
```

To run a test:

```bash
$ meson test -C build --print-errorlogs -v
```

