source_files = files(
    'huffman.c',
    'pqueue.c',
    'encoder.c',
    'decoder.c',
    'utils.c',
    'buffered-stream.c'
)

core_lib = static_library('huffman_core',source_files)

src_libs += core_lib
main_file = files('main.c')

