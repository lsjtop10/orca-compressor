source_files = files(
    'huffman.c',
    'pqueue.c',
    'encoder.c',
    'decoder.c',
    'utils.c',
    'buffered-stream.c', 
    'errors.c',
    'serializer.c',
)

# 플랫폼 별 분기
if target_platform == 'windows'
    source_files += files('file_win.c')
endif

core_lib = static_library('huffman_core',source_files)

src_libs += core_lib
main_file = files('main.c')

