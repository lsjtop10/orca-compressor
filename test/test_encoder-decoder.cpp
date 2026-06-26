#include <cstddef>
#include <cstdio>
#include <gtest/gtest.h>

extern "C" {
#include "buffered-stream.h"
#include "decoder.h"
#include "encoder.h"
#include "huffman.h"
#include "utils.h"
#include <string.h>
}

TEST(encoderDeCoder, shoudEncodeAndDecode) {
    const char* str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
                "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis "
                "nostrud exercitation ullamco laboris nisi ut aliquip ex eacommodo consequat.Duis "
                "aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu"
                "fugiatnulla pariatur.Excepteur sint occaecat cupidatat non proident, sunt in"
                "culpa qui officia deserunt mollit anim id est laborum.";

    // Build Huffman Tree
    size_t strSize = strlen(str) + 1;

    FreqTable ft;
    HuffmanCodeTable ct;

    init_FreqTable(&ft);
    init_HuffmanCodeTable(&ct);

    count_FreqTable(&ft, (uint8_t*)str, strSize);
    HuffmanTreeNode* treeRoot = build_HuffmanTree(&ft);

    HuffmanCode code;
    memset(code.bytes, 0, sizeof(code.bytes));

    buildTable_HuffmanCodeTable(&ct, treeRoot, code, 0);

    BufferedInputStream bis;
    BufferedOutputStream bos;

    // Encode
    MemoryInputStream* mis = create_MemoryInputStream();
    MemoryOutputStream* mos = create_MemoryOutStream();

    fill_MemeoryInputStream(mis, (uint8_t*)str, strSize);

    init_BufferdInputStream(&bis, mis, fetch_MemoryInputStream);
    init_BufferdOutputStream(&bos, mos, flush_MemeoryOutStream);

    StreamEncoder se = {&bis, &bos};

    encodeStream_StreamEncoder(&se, &ct);

    // Decode
    clear_MemoryInputStream(mis);
    size_t size;
    auto bytes = (uint8_t*)borrowInternalBuf_MemoryOutStream(mos, &size);
    fill_MemeoryInputStream(mis, bytes, size);
    
    destroy_MemoryOutStream(mos);
    mos = create_MemoryOutStream();

    init_BufferdInputStream(&bis, mis, fetch_MemoryInputStream);
    init_BufferdOutputStream(&bos, mos, flush_MemeoryOutStream);

    StreamDecoder sd = {&bis, &bos, strSize};

    decodeStream_StreamDecoder(&sd, treeRoot);

    bytes = takeInternalBuf_MemoryOutStream(mos, &size); 
    ASSERT_EQ(strSize, size);
    for (size_t i = 0; i < size; i++) {
        ASSERT_EQ((char)str[i], (char)bytes[i]);
    }

    destroy_MemoryInputStream(mis);
    destroy_MemoryOutStream(mos);
}