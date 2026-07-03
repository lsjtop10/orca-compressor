
# OCOMP (Orca Compressor)프로젝트 개요

2026학년 1학기를 마무리하면서 배운 내용을 심화 / 응용하기 위해 작성한 허프만 코딩 압축 프로그램 프로젝트입니다. 학기말 과제를 완성한다고 생각하고 작성했습니다.

## 관련 과목

- **자료구조**: 우선순위 큐, 이진트리의 표현 및 연산.
- **객체지향프로그래밍및실습**: 캡슐화, 다형성, 
  
## 요구사항 및 목표

1. **개발 언어**: 자료구조 수업 시간에 사용한 C언어를 그대로 사용합니다.
2. **파일 입출력**: 원본 파일에서 허프만 압축이 적용된 바이너리 출력과 허프만 바이너리 출력에서 원본 파일 출력 두 기능 모두 구현합니다.
3. **객체지향 패러다임 적용**: C언어의 다양한 특성을 적용해서 객체지향 패러다임을 적용합니다.(불투명 포인터, 함수 네이밍 규칙)
4. **에러 처리 및 방어적 프로그래밍**: C언어에서 계층적 에러 처리 방향을 고민하고 적용합니다.
5. **소유권 관리**: RAII, 빌림 검사 등 타 언어의 소유권 처리를 모방합니다.

## 생성형 AI 사용 기준

main.c, file.h, 테스트 코드, 주석은 AI가 주도로 작성했습니다. 주요 알고리즘이 아니면서 코드량이 많은 부분에 일부 적용했습니다. 이외 구조 설계에 채팅형 AI의 도움을 받았습니다.

# 프로젝트 종속성 및 환경

## 정적 분석 도구 및 언어 서버

- **clangd**
- **clang-tidy**

## 빌드 시스템

- **Meson** (>= 0.60.0)
- **Ninja**
- **GCC / G++**

## 테스트 프레임워크

- **Google Test (GTest)**

## 개발 및 테스트 완료 환경

- **Windows:** MSYS2 MINGW-X64-UCRT (Universal C Runtime) 환경

## 🚀 시작하기

## 1. 의존성 설치 (Windows MSYS2 기준)

MSYS2 터미널에서 열고 다음 패키지들을 설치합니다.

``` bash
pacman -S mingw-w64-ucrt-x86_64-meson mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gtest mingw-w64-ucrt-x86_64-g++ mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-gdb
```

## 2. 빌드 디렉토리 설정

``` bash
meson setup builddir
```

## 3. 컴파일 및 빌드

``` bash
meson compile -C build
```

## 4. 테스트 실행

``` bash
meson test -C build
```


This project is licensed under the MIT License - see the LICENSE file for details.