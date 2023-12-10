
project('FigureGenerator', 'cpp',
	default_options: [
		'backend=vs2022',
		'buildtype=release',
		'cpp_std=c++17',
		'warning_level=3',
	]
)

main = executable('main',
	[
		'FigureGenerator.hpp',
		'BS_thread_pool.hpp',
		'main.cpp',
	],
	# Modify this line to allow bigger figures.
	cpp_args: [ '-DNMAX=20' ],
)

