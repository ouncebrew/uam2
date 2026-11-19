uam2_sources = files(
	'main.cpp',
	'compile.c',
	'dksh_writer.cpp',
	'glsl_frontend.cpp',
	'lower_abi.c',
	't239.c',
)

if host_machine.system() == 'windows'
	uam2_sources += files(
		'windows-compat/mman_stub.c',
		'windows-compat/drm_stub.c',
	)
endif

uam2 = executable('uam2',
	uam2_sources,
	c_args: mesa_args,
	cpp_args: mesa_args,
	dependencies: mesa_deps,
	install: true,
)
