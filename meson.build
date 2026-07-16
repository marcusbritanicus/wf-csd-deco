project(
	'wf-decorator',
    'c',
	'cpp',
	version: '0.0',
	license: 'MIT',
	meson_version: '>=0.51.0',
	default_options: [
		'cpp_std=c++17',
        'c_std=c99',
		'werror=false',
	],
)

gtk4 = dependency('gtk4', version: '>=4.22.0')
wayfire = dependency('wayfire', version: '>=0.11.0')
wayland_client = dependency('wayland-client')
wayland_server = dependency('wayland-server')

project_args = ['-DWLR_USE_UNSTABLE']
add_project_arguments(project_args, language: ['cpp', 'c'])

subdir('proto')
subdir('metadata')
subdir('wf-plugin')
subdir('wf-gtk4-client')
