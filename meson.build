project('usbflyenc', 'c',
        version : '1.0.0',
        default_options : ['warning_level=3', 'c_std=c17'])

sodium = dependency('libsodium', required : true)

files = ['src/main.c', 'src/crypto.c', 'src/crypto.h']

usbflyenc = executable('usbflyenc', files, dependencies : [sodium], install : true)

test('test', usbflyenc)