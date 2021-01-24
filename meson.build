project('tittut', 'cpp')

cc = meson.get_compiler('cpp')
cpp_args = ['-std=c++17', '-Wall', '-Wextra']

if get_option('buildtype').startswith('release')
    add_project_arguments('-DNDEBUG', language : ['cpp'])
endif

tittut_inc = include_directories('tittut')

subdir('tittut')


