project('huffman_compression', 'c',
  version : '1.0.0',
  default_options : [
    'c_std=c11',           # C11 표준 사용
    'warning_level=3',     # 경고 컴파일 에러 수준을 높임 (엄격한 코드 관리)
    'werror=false'         # 경고를 에러로 처리할지 여부
  ]
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

source_files = []

inc_dirs = include_directories(
  'src',
  is_system: false,
)

subdir('src')

executable('huffman_compress',
 source_files,
dependencies: [m_dep],
install: true,
include_directories: inc_dirs)

