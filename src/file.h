#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include <stdint.h>
#include "ownership.h"
#include "errors.h"

/**
 * @brief PlatformFileCtx 구조체는 플랫폼별 파일 입출력 컨텍스트를 추상화한 구조체입니다.
 * @details 이 구조체는 파일 입출력에 필요한 정보를 담고 있으며, 플랫폼에 따라 구현이 달라질 수 있습니다.
 *          따라서 이 구조체의 내부 구현은 플랫폼별로 다르게 작성되어야 합니다.
 */
typedef struct PlatformFileCtx PlatformFileCtx;

// 로우레벨 함수는 플랫폼별로 구현되어야 하며, 상위 계층에서는 플랫폼 독립적인 인터페이스를 제공한다.
// 성능 오버헤드 감소와 반복문 호출 용이성을 위해 try계열 함수로 주로 구성. 

// 파일 오픈 및 포인터 이동 인터페이스
Move(PlatformFileCtx*) open_PlatformFileCtx(const char utf8Path[], const char mode[], ErrorContext* err);
bool trySeek_PlatformFileCtx(PlatformFileCtx* ctx, uint64_t offset, ErrorContext* err);
bool tryMove_PlatformFileCtx(PlatformFileCtx* ctx, int delta, ErrorContext* err);
uint64_t getFileSize_PlatformFileCtx(PlatformFileCtx* ctx, ErrorContext* err);

// 순수 바이트 읽기 인터페이스
size_t readBytes_PlatformFileCtx(PlatformFileCtx* ctx, uint8_t ptr[], size_t maxSize, ErrorContext* err);

// 스트림 추상화 연동을 위한 인라인 함수
static inline size_t fetch_PlatformFileCtx(const void* stream, uint8_t buf[], size_t bufSize, ErrorContext* err) {
    return readBytes_PlatformFileCtx((PlatformFileCtx*)stream, buf, bufSize, err);
}

// 순수 바이트 쓰기 인터페이스
size_t writeBytes_PlatformFileCtx(PlatformFileCtx* ctx, const uint8_t ptr[], size_t size, ErrorContext* err);

static inline size_t flush_PlatformFileCtx(const void* stream, uint8_t buf[], size_t offset, size_t bufSize, ErrorContext* err) {
    return writeBytes_PlatformFileCtx((PlatformFileCtx*)stream, buf + offset, bufSize, err);
}

void close_PlatformFileCtx(Move(PlatformFileCtx*) f);

typedef uint8_t Algorithm; 

enum {
    ALGORITHM_HUFFMAN,
};

// 시그니처는 링크 에러 방지를 위해 매크로 상수로 정의합니다.
#define FILE_SIGNATURE "OCOP"
#define CURRENT_ARCHIVE_FORMAT_VERSION 1

typedef struct {
    int8_t signature[4];
    uint8_t version;
    Algorithm algorithm;
    uint16_t originalNameLength;
    // Ownership: own. 구조체 뒤에 동적 할당 영역으로 붙는 가변 배열입니다.
    // readFileMeta 호출 시 전체 크기를 계산하여 한 번에 할당해야 합니다.
    char name[]; 
} FileMetaV1;

typedef struct {
    uint64_t size;
    uint8_t version;
    uint64_t originalSize;
    uint32_t treeSize;
} ChunkInfoV1;


// 메타데이터 및 청크 헬퍼 인터페이스 (I/O 독립 기능)
FileMetaV1* fillFileMeta_FileMetaV1(PlatformFileCtx* ctx, ErrorContext* err);
void destroyFileMeta_FileMetaV1(FileMetaV1* meta);

#endif