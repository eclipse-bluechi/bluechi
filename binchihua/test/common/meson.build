# common test configuration
parse_util_src = [
  'parse-util_test.c'
]

list_src = [
  'list_test.c'
]

parse_util_src += common_src
list_src += common_src

e = executable('parse-util_test', parse_util_src)
test('parse-util_test', e)

e = executable('list_test', list_src)
test('list_test', e)