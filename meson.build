project('ukb', 'c',
  version: '0.1.0',
  default_options: ['warning_level=2', 'optimization=2'],
)


inc = include_directories('include')
lib_src = files('src/ukb.c')
deps = []

platform = host_machine.system()

if platform == 'linux'
  add_project_arguments('-DPLATFORM_LINUX', language: ['c'])
  lib_src += files(
    'src/backends/sway.c',
    'src/backends/xorg.c',
  )
  deps += [
    dependency('json-c', required: true),
    dependency('x11', required: true),
  ]
elif platform == 'windows'
  add_project_arguments('-DPLATFORM_WINDOWS', language: ['c'])
  lib_src += files('src/backends/windows.c')
else
  error('unknown platform: ' + platform)
endif


install_headers('include/ukb.h', subdir: 'ukb')

libukb_kwargs = {
  'include_directories': inc,
  'dependencies': deps,
  'c_args': ['-DUKB_BACKENDS_INTERNAL'],
  'install': true,
}
libukb_a = static_library('ukb', lib_src, kwargs: libukb_kwargs)
libukb_so = shared_library('ukb', lib_src, kwargs: libukb_kwargs)


executable('ukb', 'src/main.c',
  include_directories: inc,
  link_with: libukb_a,
  install: true,
)
