#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED
#include <stdint.h>
#include "ownership.h"
#include "errors.h"


typedef enum {
    HUFFMAN,
}Algolithem;

const char* signiture = "OCMP";

typedef struct{
    int8_t signiture[4];
    uint8_t version;

    uint8_t algolithem;
    // 파일 이름 인코딩 코드.
    uint16_t FilneNameLengh;
}FileMetaV1;

typedef struct{
    // 청크 전체 사이즈(byte). 허프만 트리 포함.
    uint64_t size;
    uint8_t version;
    // 원본 데이터 사이즈(byte). 청크 전체 사이즈보다 원본 사이즈가 더 커질 수 있음.
    uint64_t originalSize;
    //허프만 트리 크기(byte)
    uint32_t treeSize;
}ChunkInfoV1;

typedef struct PlatformFileCtx PlatformFileCtx;

/**
* @brief open file 
*/
Move(PlatformFileCtx*) open_PlatformFileCtx(const char* utf8Name, const char* mode);

//
void readData_PlatformFileCtx(PlatformFileCtx* ctx, void* ptr, size_t size, Mut(Error*) err);

size_t readBytes_PlatformFileCtx(PlatformFileCtx* ctx, uint8_t* ptr, size_t maxSize, Mut(Error*) err);
uint8_t readByte_PlatformFileCtx(PlatformFileCtx* ctx, Mut(Error*) err);
uint8_t readChar_PlatformFileCtx(PlatformFileCtx* ctx, Mut(Error*) err);
void readString_PlatformFileCtx(Borrow(char*) utf8StrPtr, size_t length, Mut(Error*) err);


void writeData_PlatformFileCtx(uint8_t* ptr, size_t size, Mut(Error*) err);
size_t writeBytes_PlatformFileCtx(uint8_t* ptr, size_t size, Mut(Error*) err);
void writeByte_PlatformFileCtx(uint8_t* ptr, size_t size, Mut(Error*) err);
void writeChar_PlatformFileCtx(uint16_t c, Mut(Error*) err);

// UTF-8인코딩 된 
void writeString_PlatformFileCtx(Borrow(char*) utf8StrPtr, size_t length);


FileMetaV1 readFileMeta(Borrow(PlatformFileCtx*) ctx);



int close(Move(PlatformFileCtx*) f);
#endif