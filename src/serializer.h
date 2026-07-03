#ifndef SERIALIZER_H_INCLUDED
#define SERIALIZER_H_INCLUDED

#include "file.h"
#include "huffman.h"
#include "buffered-stream.h"
#include "errors.h"

// FileMetaV1 직렬화/역직렬화
void writeFileMeta_Serializer(BufferedOutputStream* bos, FileMetaV1* meta, ErrorContext* err);
FileMetaV1* readAndCreateFileMeta_Serializer(BufferedInputStream* bis, ErrorContext* err);

// ChunkInfoV1 직렬화/역직렬화
void writeChunkInfo_Serializer(BufferedOutputStream* bos, ChunkInfoV1* chunk, ErrorContext* err);
void readChunkInfo_Serializer(BufferedInputStream* bis, ChunkInfoV1* chunk, ErrorContext* err);

// HuffmanTree 직렬화/역직렬화
void serializeTree_Serializer(BufferedOutputStream* bos, Borrow(HuffmanTreeNode*) htRoot, ErrorContext* err);
HuffmanTreeNode* deserializeTree_Serializer(BufferedInputStream* bis, uint32_t treeSize, ErrorContext* err);

#endif
