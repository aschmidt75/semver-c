project('semver', 'c',
  version : '0.2',
  default_options : ['warning_level=3','c_std=c89'])

inc_dirs = include_directories('./include', './vendor/Unity/src')

t = executable(
  'test-semver', 
  'vendor/Unity/src/unity.c',
  'src/semver.c', 'test/semver-test.c',
  'test/main-test.c',
  include_directories: inc_dirs,
  install : false)

test('unit-tests', t)
