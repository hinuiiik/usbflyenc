project('usbflyenc', 'c', 'cpp',
        version : '1.0.0',
        default_options : ['warning_level=3', 'c_std=c17', 'cpp_std=c++17']) # Added C++ standard for GoogleTest

sodium = dependency('libsodium', required : true)
glib = dependency('glib-2.0', required : true)
gio =  dependency('gio-2.0', required : true)

inc = include_directories('src', 'include')
src_files = files('src/main.c', 'src/crypto.c', 'src/dbus.c', 'src/manifest.c', 'src/inotify.c', 'src/drive.c', 'src/workspace.c')

usbflyenc = executable('usbflyenc', src_files,
                       include_directories : inc,
                       dependencies : [sodium, glib, gio],
                       install : true)


gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_main_dep')

test_sources = files('tests/test_crypto.cpp', 'src/crypto.c')
test_exe = executable('gtest_runner',
                      test_sources,
                      include_directories : inc,
                      dependencies : [gtest_dep, sodium]
)

test('unit_tests',
     test_exe,
     protocol: 'gtest',
     env : [
         'TEST_DATA_DIR=' + meson.current_source_dir() / 'tests/data',
     ],
)
