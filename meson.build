project(
  'kindle-custom-ss',
  'cpp',
  version: 'v1.0.0',
  default_options: ['cpp_std=c++17'],
  meson_version: '>=1.1'
)

cc = meson.get_compiler('cpp')

pango_dep = dependency('pango')
cairo_dep = dependency('cairo')
thread_dep = dependency('threads')
glib_dep = dependency('glib-2.0')

openlipc_proj = subproject('openlipc')
openlipc_dep = openlipc_proj.get_variable('libopenlipc_dep')

# Create a custom dependency for the blanket library
kindle_root = get_option('kindle_root_dir')
blanket_dep = declare_dependency(
  dependencies: cc.find_library('blanket', dirs: join_paths(kindle_root, 'usr/lib'), required: true)
)
llog_dep = declare_dependency(
  dependencies: cc.find_library('llog', dirs: join_paths(kindle_root, 'usr/lib'))
)

sources = files(
  './src/screensaver.cpp',
)

include_dirs = include_directories(
  './src/include/',
)

shared_library(
  'custom',
  sources,
  include_directories: include_dirs,
  dependencies: [
    openlipc_dep,
    llog_dep,
    glib_dep,
    cairo_dep,
    thread_dep,
    pango_dep,
    blanket_dep,
  ],
  # https://kindlemodding.org/kindle-dev/gtk-tutorial/setting-up.html
  #   Koxtoolchain ships with a GCC version newer than the Kindle technically
  #   supports, but this is fine so long as we statically link the standard
  #   library.
  cpp_args: '-static-libstdc++',
  link_args: ['-static-libstdc++']
)
