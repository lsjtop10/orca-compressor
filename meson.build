project(
  'ocomp',
  'c',
  'cpp',
  version: '1.0.0',
  default_options: [
    'c_std=c11',
    'cpp_std=c++17',
    'warning_level=3',
    'werror=false',
  ],
)

cc = meson.get_compiler('c')

if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  add_project_arguments('-Wno-ignored-qualifiers', language : 'c')
endif

# 의존성 및 인크루드 경로 확보
gtest_dep = dependency('gtest', main: true, fallback: ['gtest', 'gtest_main_dep'], default_options: ['default_library=static'])
m_dep = cc.find_library('m', required: false)

inc_dirs = include_directories('src', is_system: false)

# 이 프로그램이 실행될 환경을 가져온다.
target_platform = host_machine.system()

# 윈도우일 때만 리소스
if target_platform == 'windows'
  # 💡 윈도우용 리소스 컴파일러(windres) 호출기 작동
  win = import('windows')
  res = win.compile_resources('src/platform/huffman.rc')
endif

# 하위 모듈(오브젝트/라이브러리)들을 모을 전역 배열 선언
src_libs = []

# src 디렉토리 진입 
# (이 안에서 core_sources 등을 빌드하여 src_libs 배열에 정적 라이브러리 객체들을 누적해야 합니다)
subdir('src')
subdir('test')

# 구글 테스트 실행 파일 빌드
# (따로 빌드 타깃을 선언하지 않고, 메인 파일과 공유하는 src_libs만 그대로 링크해 줍니다)
test_exe = executable(
'run_tests',
  test_sources,
  dependencies: [gtest_dep, m_dep],
  link_with: src_libs,
  link_language: 'cpp',
  include_directories: inc_dirs,
)

test('run_test', test_exe)

# 메인 실행 파일 빌드 (src_libs에 쌓인 C 라이브러리들을 묶어서 빌드)
executable(
  'ocomp',
  main_file, # src/meson.build 내부에서 정의되어 넘어와야 함
  dependencies: [m_dep],
  link_with: src_libs, 
  install: true,
  include_directories: inc_dirs,
)
