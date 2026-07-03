#include <cstddef>
#include <cstdio>
#include <gtest/gtest.h>
#include <random>

extern "C" {
#include "buffered-stream.h"
#include "decoder.h"
#include "encoder.h"
#include "huffman.h"
#include "utils.h"
#include <string.h>
}

namespace {

static void TestEncoderAndDecoder(uint8_t* data, size_t dataSize) {
    // Build Huffman Tree
    FreqTable ft;
    HuffmanCodeTable ct;

    init_FreqTable(&ft);
    init_HuffmanCodeTable(&ct);

    accumulate_FreqTable(&ft, data, dataSize);
    HuffmanTreeNode* treeRoot = build_HuffmanTree(&ft);

    HuffmanCode code;
    memset(code.bytes, 0, sizeof(code.bytes));

    buildTable_HuffmanCodeTable(&ct, treeRoot, code, 0);

    BufferedInputStream* bis;
    BufferedOutputStream* bos;
    ErrorContext* err = create_ErrorContext();

    // Encode
    MemoryInputStream* mis = create_MemoryInputStream();
    MemoryOutputStream* mos = create_MemoryOutputStream();

    fill_MemeoryInputStream(mis, data, dataSize);

    bis = create_BufferedInputStream(mis, fetch_MemoryInputStream);
    bos = create_BufferedOutputStream(mos, flush_MemoryOutputStream);

    StreamEncoder se = {bis, bos};

    encodeStream_StreamEncoder(&se, &ct, err);
    ASSERT_EQ(peekSurfaceError_ErrorContext(err), nullptr);

    // Decode
    clear_MemoryInputStream(mis);
    size_t size;
    auto bytes = (uint8_t*)borrowInternalBuf_MemoryOutputStream(mos, &size);
    fill_MemeoryInputStream(mis, bytes, size);

    destroy_MemoryOutputStream(mos);
    mos = create_MemoryOutputStream();

    reset_BufferedInputStream(bis, mis, fetch_MemoryInputStream);
    reset_BufferedOutputStream(bos, mos, flush_MemoryOutputStream);

    StreamDecoder sd = {bis, bos, dataSize};

    decodeStream_StreamDecoder(&sd, treeRoot, err);

    // Assert
    ASSERT_EQ(peekSurfaceError_ErrorContext(err), nullptr);
    bytes = (uint8_t*)borrowInternalBuf_MemoryOutputStream(mos, &size);
    ASSERT_EQ(dataSize, size);
    for (size_t i = 0; i < size; i++) {
        ASSERT_EQ(data[i], bytes[i]);
    }

    // Destroy
    destroy_MemoryInputStream(mis);
    destroy_MemoryOutputStream(mos);
    destroy_BufferedInputStream(bis);
    destroy_BufferedOutputStream(bos);
    destroy_ErrorContext(err);
    destroy_HuffmanTreeNode(treeRoot);
}

static void TestEncoder(uint8_t* data, size_t dataSize, uint8_t* expected, uint8_t expectedSize) {
    FreqTable ft;
    HuffmanCodeTable ct;

    init_FreqTable(&ft);
    init_HuffmanCodeTable(&ct);

    accumulate_FreqTable(&ft, data, dataSize);
    HuffmanTreeNode* treeRoot = build_HuffmanTree(&ft);

    HuffmanCode code;
    memset(code.bytes, 0, sizeof(code.bytes));

    buildTable_HuffmanCodeTable(&ct, treeRoot, code, 0);

    for (size_t i = 0; i < dataSize; i++) {
        HuffmanCode code = lookupCode_HuffmanCodeTable(&ct, data[i]);
        size_t length = lookupCodeLength_HuffmanCodeTable(&ct, data[i]);
        for (size_t j = 0; j < length; j++) {
            if (getBit_HuffmanCode(code, j)) {
                putchar('1');
            } else {
                putchar('0');
            }
        }
    }

    BufferedInputStream* bis;
    BufferedOutputStream* bos;

    // Encode
    MemoryInputStream* mis = create_MemoryInputStream();
    ASSERT_NE(mis, nullptr);
    MemoryOutputStream* mos = create_MemoryOutputStream();
    ASSERT_NE(mos, nullptr);
    ErrorContext* err = create_ErrorContext();
    ASSERT_NE(err, nullptr);


    fill_MemeoryInputStream(mis, data, dataSize);

    bis = create_BufferedInputStream(mis, fetch_MemoryInputStream);
    bos = create_BufferedOutputStream(mos, flush_MemoryOutputStream);

    StreamEncoder se = {bis, bos};

    encodeStream_StreamEncoder(&se, &ct, err);

    ASSERT_EQ(peekSurfaceError_ErrorContext(err), nullptr) <<
        "Error: " << getMsg_Error(peekSurfaceError_ErrorContext(err)) <<
            "code: "<< getCode_Error(peekSurfaceError_ErrorContext(err)) << "\n";

    size_t actualSize = 0;
    const uint8_t* actual = borrowInternalBuf_MemoryOutputStream(mos, &actualSize);
    ASSERT_EQ(expectedSize, actualSize);
    for (size_t i = 0; i < actualSize; i++) {
        ASSERT_EQ(expected[i], actual[i]) << i;
    }

    destroy_MemoryInputStream(mis);
    destroy_MemoryOutputStream(mos);
    destroy_BufferedInputStream(bis);
    destroy_BufferedOutputStream(bos);
    destroy_ErrorContext(err);
    destroy_HuffmanTreeNode(treeRoot);
}

} // namespace

TEST(encoderDecoder, shoudEncodeAndDecodeLoremIpsum) {
    const char* str = "Lorem ipsum";

    // Build Huffman Tree
    size_t strSize = strlen(str) + 1;
    TestEncoderAndDecoder((uint8_t*)str, strSize);

}


TEST(encoderDecoder, shoudEncodeAndDecode) {
    const char* str =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
        "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis "
        "nostrud exercitation ullamco laboris nisi ut aliquip ex eacommodo consequat.Duis "
        "aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu"
        "fugiatnulla pariatur.Excepteur sint occaecat cupidatat non proident, sunt in"
        "culpa qui officia deserunt mollit anim id est laborum.";

    // Build Huffman Tree
    size_t strSize = strlen(str) + 1;

    TestEncoderAndDecoder((uint8_t*)str, strSize);
}

TEST(encoderDecoder, shoudEncodeBigData) {

    size_t dataSize = 4096;
    std::vector<uint8_t> bigData(dataSize);

    std::mt19937 gen(1);
    std::uniform_int_distribution<int> dis(0, 255);

    for (size_t i = 0; i < dataSize; i++) {
        bigData[i] = dis(gen);
    }

    TestEncoderAndDecoder(bigData.data(), dataSize);
}

TEST(encoderDecoder, shoudEncodeBigData2) {

    size_t dataSize = 1024 * 1024 * 16;
    std::vector<uint8_t> bigData(dataSize);

    std::mt19937 gen(1);
    std::uniform_int_distribution<int> dis(0, 255);

    for (size_t i = 0; i < dataSize; i++) {
        bigData[i] = dis(gen);
    }

    TestEncoderAndDecoder(bigData.data(), dataSize);
}

TEST(encoder, shoudEncodeLoremIpsum) {
    const char* str = "Lorem ipsum";
    uint8_t expected[6] = {0x21, 0x44, 0xFC, 0x1A, 0xA3, 0xC0};

    TestEncoder((uint8_t*)str, strlen(str) + 1, expected, sizeof(expected) / sizeof(uint8_t));
}

TEST(encoder, shoudEncodeLoremIpsumExeptForNullChar) {
    const char* str = "Lorem ipsum";
    uint8_t expected[5] = {0xD1, 0x06, 0x00, 0xCC, 0x4E};

    TestEncoder((uint8_t*)str,
        strlen(str), expected,
        sizeof(expected) / sizeof(uint8_t));
}
