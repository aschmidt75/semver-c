project('semver', 'c',
  version : '0.4',
  default_options : [
    'warning_level=3',
    'c_std=c89',
    'b_sanitize=address'
  ])

add_global_arguments(['-Wno-pointer-sign',
                      '-Wno-deprecated-declarations',
                      '-Wno-unused',
                      '-Wno-unused-parameter',
                      '-Wno-incompatible-pointer-types'],
                      language: 'c')

semver_lib = static_library('semver',
  'src/semver.c',
  'src/semverreq.c',
  include_directories: [ './include' ]
)

unity_lib = static_library('unity',
  'vendor/Unity/src/unity.c',
  include_directories: [ './vendor/Unity/src' ]
)

t = executable(
  'test-semver',
  'test/semver-test.c',
  'test/semverreq-test.c',
  'test/main-test.c',
  include_directories: [ './include', './vendor/Unity/src' ],
  link_with: [ semver_lib, unity_lib ],
  install : false)

test('unit-tests', t)

