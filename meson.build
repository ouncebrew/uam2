project('uam2', ['c', 'cpp'],
	version: '0.1.0',
	meson_version: '>= 1.7.0',
	default_options: [ 'buildtype=release', 'b_ndebug=false', 'c_std=c11', 'cpp_std=c++17' ],
)

add_global_arguments('-I' + meson.global_build_root(), language: 'c')

# Keep in sync with subprojects/mesa.wrap
mesa_version = '26.3.0'

mesa = subproject('mesa',
	default_options: [
		'nak-standalone=true',
		'platforms=',
		'gallium-drivers=',
		'vulkan-drivers=',
		'opengl=true',
		'gles1=disabled',
		'gles2=disabled',
		'glx=disabled',
		'egl=disabled',
		'gbm=disabled',
		'llvm=disabled',
		'video-codecs=',
		'tools=',
		'build-tests=false',
		'werror=false',
	],
)

mesa_deps = [
	mesa.get_variable('idep_nak'),
	mesa.get_variable('idep_vtn'),
	mesa.get_variable('idep_nir'),
	mesa.get_variable('idep_libglsl'),
	mesa.get_variable('idep_mesautil'),
	mesa.get_variable('idep_nvidia_headers').partial_dependency(includes: true),
	declare_dependency(
		include_directories: [
			mesa.get_variable('inc_spirv'),
			mesa.get_variable('inc_glsl'),
			mesa.get_variable('inc_mesa'),
			mesa.get_variable('inc_gallium'),
			mesa.get_variable('inc_gallium_aux'),
		],
		link_with: mesa.get_variable('libglsl_standalone'),
	),
]

mesa_args = mesa.get_variable('pre_args') + [
	'-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
	'-DUAM2_MESA_VERSION="@0@"'.format(mesa_version),
]

subdir('source')
