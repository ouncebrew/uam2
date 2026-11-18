if runtime
	uam2rt_api = ['uam2rt_init', 'uam2rt_exit', 'uam2rt_compile_spirv']

	uam2rt_link_args = ['-nostdlib', '-r', '-Wl,--force-group-allocation', '-Wl,--gc-sections']
	foreach sym : uam2rt_api
		uam2rt_link_args += '-Wl,--undefined=' + sym
	endforeach

	uam2rt_obj = executable('uam2rt.o',
		files(
			'compile.c',
			'dksh_writer.cpp',
			'lower_abi.c',
			'horizon.c',
			'runtime.c',
			't239.c',
		),
		c_args: mesa_args + '-DUAM2_RUNTIME',
		cpp_args: mesa_args + '-DUAM2_RUNTIME',
		include_directories: include_directories('../include'),
		dependencies: mesa_deps,
		link_args: uam2rt_link_args,
	)

	objcopy = find_program('objcopy')
	ar = find_program('ar')
	keep_args = []
	foreach sym : uam2rt_api
		keep_args += '--keep-global-symbol=' + sym
	endforeach

	uam2rt = custom_target('libuam2rt.a',
		input: uam2rt_obj,
		output: 'libuam2rt.a',
		command: [find_program('package_runtime.sh'), objcopy, ar, '@INPUT@', '@OUTPUT@', keep_args],
		install: true,
		install_dir: get_option('libdir'),
	)
	install_headers('../include/uam2rt.h')
else
	uam2 = executable('uam2',
		files(
			'main.cpp',
			'compile.c',
			'dksh_writer.cpp',
			'glsl_frontend.cpp',
			'lower_abi.c',
			't239.c',
		),
		c_args: mesa_args,
		cpp_args: mesa_args,
		dependencies: mesa_deps,
		install: true,
	)
endif
