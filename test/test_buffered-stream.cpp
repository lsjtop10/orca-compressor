#include <gtest/gtest.h>
#include <random>

extern "C"{
    #include "buffered-stream.h"
    #include "utils.h"
}


TEST(bufferedOutputStream, shouldWriteString) {

    const char* testStr = "Hello, World!";
    size_t dataSize = strlen(testStr) + 1;

    ErrorContext* err = create_ErrorContext();
    ASSERT_NE(err, nullptr);
    MemoryOutputStream* mos = create_MemoryOutputStream();
    ASSERT_NE(mos, nullptr);
    BufferedOutputStream* bos = create_BufferedOutputStream(mos, flush_MemoryOutputStream);
    ASSERT_NE(bos, nullptr);
    
    for (size_t i = 0; i < dataSize; i++) {
        bool success = tryWriteByte_BufferedOutputStream(bos, testStr[i], err);
        ASSERT_EQ(success, true);
    }

    flush_BufferedOutputStream(bos, err);

    
    size_t actualSize;
    auto actual = (uint8_t*)borrowInternalBuf_MemoryOutputStream(mos, &actualSize);
    
    ASSERT_EQ(dataSize, actualSize);
    for (size_t i = 0; i < dataSize; i++) {
        ASSERT_EQ(testStr[i], actual[i]);
    }

    destroy_MemoryOutputStream(mos);
    destroy_BufferedOutputStream(bos);
}

TEST(bufferedOutputStream, shouldWriteBitsAndBytes) {

    auto dataSize = (size_t)(1024 * 1024 * 16);
    std::vector<uint8_t> bigData(dataSize);

    std::mt19937 gen(1);
    std::uniform_int_distribution<int> dis(0, 255);

    for (size_t i = 0; i < dataSize; i++) {
        bigData[i] = dis(gen);
    }

    ErrorContext* err = create_ErrorContext();
    ASSERT_NE(err, nullptr);
    MemoryOutputStream* mos = create_MemoryOutputStream();
    ASSERT_NE(mos, nullptr);
    BufferedOutputStream* bos = create_BufferedOutputStream(mos, flush_MemoryOutputStream);
    ASSERT_NE(bos, nullptr);
    
    for (size_t i = 0; i < dataSize; i++) {
        bool success = tryWriteByte_BufferedOutputStream(bos, bigData[i], err);
        ASSERT_EQ(success, true);
    }

    flush_BufferedOutputStream(bos, err);

    
    size_t actualSize;
    auto actual = (uint8_t*)borrowInternalBuf_MemoryOutputStream(mos, &actualSize);
    
    ASSERT_EQ(dataSize, actualSize);
    for (size_t i = 0; i < dataSize; i++) {
        ASSERT_EQ(bigData[i], actual[i]);
    }

    destroy_MemoryOutputStream(mos);
    destroy_BufferedOutputStream(bos);
}


TEST(bufferedInputStream, shouldReadString) {
    const char* testStr = "Hello, World!";
    size_t dataSize = strlen(testStr) + 1;

    BufferedInputStream* bis;
    ErrorContext* err = create_ErrorContext();
    MemoryInputStream* mis = create_MemoryInputStream();

    fill_MemeoryInputStream(mis, (uint8_t*)testStr, dataSize);
    bis = create_BufferedInputStream(mis, fetch_MemoryInputStream);

    for (size_t i = 0; i < dataSize; i++) {
        uint8_t actual;
        bool success = tryNextByte_BufferedInputStream(bis,&actual, err);
        ASSERT_EQ(success, true);
        ASSERT_EQ(testStr[i], actual);
    }

    destroy_MemoryInputStream(mis);
    destroy_BufferedInputStream(bis);
    destroy_ErrorContext(err);
}

TEST(bufferedInputStream, shouldReadBitsAndBytes) {

    auto dataSize = (size_t)(1024 * 1024 * 16);
    std::vector<uint8_t> bigData(dataSize);

    std::mt19937 gen(1);
    std::uniform_int_distribution<int> dis(0, 255);

    for (size_t i = 0; i < dataSize; i++) {
        bigData[i] = dis(gen);
    }

    BufferedInputStream* bis;
    ErrorContext* err = create_ErrorContext();
    MemoryInputStream* mis = create_MemoryInputStream();

    fill_MemeoryInputStream(mis, bigData.data(), dataSize);
    bis = create_BufferedInputStream(mis, fetch_MemoryInputStream);

    for (size_t i = 0; i < dataSize; i++) {
        uint8_t actual;
        bool success = tryNextByte_BufferedInputStream(bis,&actual, err);
        ASSERT_EQ(success, true);
        ASSERT_EQ(bigData[i], actual);
    }

    destroy_MemoryInputStream(mis);
    destroy_BufferedInputStream(bis);
    destroy_ErrorContext(err);
}


TEST(BufferedStreamUnit, ShouldMaintainDataIntegrityOnStreamPipelineReset) {
    // 1. 순수 단위 테스트용 데이터셋 준비 (1KB 원본 데이터)
    size_t dataSize = 1024;
    std::vector<uint8_t> srcData(dataSize);
    
    std::mt19937 gen(42); // 재현 가능성을 위해 시드 고정
    std::uniform_int_distribution<int> dis(0, 255);
    for (size_t i = 0; i < dataSize; i++) {
        srcData[i] = dis(gen);
    }

    ErrorContext* err = create_ErrorContext();

    // 2. 최하부 물리 메모리 스트림 생성 (소유권 관리 대상)

    MemoryInputStream* mis = create_MemoryInputStream();
    MemoryOutputStream* mos = create_MemoryOutputStream();
    ASSERT_NE(mis, nullptr);
    ASSERT_NE(mos, nullptr);


    // 3. 상위 버퍼 스트림 객체 생성 및 래핑
    BufferedInputStream* bis = create_BufferedInputStream(mis, fetch_MemoryInputStream);
    BufferedOutputStream* bos = create_BufferedOutputStream(mos, flush_MemoryOutputStream);
    ASSERT_NE(bis, nullptr);
    ASSERT_NE(bos, nullptr);


    // STEP 1: [InputStream -> OutputStream] 데이터 파이프라이닝

    // 물리 입력 버퍼에 원본 난수 적재
    fill_MemeoryInputStream(mis, (uint8_t*)srcData.data(), dataSize);

    // BufferedInputStream에서 바이트를 한 부 씩 읽어 BufferedOutputStream으로 복사
    for (size_t i = 0; i < dataSize; i++) {
        uint8_t byte;
        bool success = tryNextByte_BufferedInputStream(bis, &byte, err); // 규칙 반영 접두사 가상 매핑
        ASSERT_EQ(success, true);
        success = tryWriteByte_BufferedOutputStream(bos, byte, err);
        ASSERT_EQ(success, true);
    }

    // 출력 버퍼의 잔여 데이터를 하부 MemoryOutputStream으로 완전히 방출
    flush_BufferedOutputStream(bos, err);
    ASSERT_EQ(peekSurfaceError_ErrorContext(err), nullptr);

    // 하부 출력 버퍼의 내부 주소 및 크기 확보 (소유권 대여)
    size_t intermediateSize = 0;
    uint8_t* intermediateBytes = (uint8_t*)borrowInternalBuf_MemoryOutputStream(mos, &intermediateSize);
    ASSERT_EQ(dataSize, intermediateSize);


    // STEP 2: [OutputStream -> InputStream] 자원 재사용 및 크로스 리셋

    // 물리 입력 스트림을 비우고, 방금 출력된 따끈따끈한 데이터를 다시 입력단에 주입
    clear_MemoryInputStream(mis);
    fill_MemeoryInputStream(mis, intermediateBytes, intermediateSize);

    // 물리 출력 스트림은 검증용 최종 결과 적재를 위해 파괴 후 리네임 재생성
    destroy_MemoryOutputStream(mos);
    mos = create_MemoryOutputStream();

    // ★ 단위 테스트 핵심: 기존에 가동되었던 버퍼 스트림 객체들을 완벽히 초기화
    reset_BufferedInputStream(bis, mis, fetch_MemoryInputStream);
    reset_BufferedOutputStream(bos, mos, flush_MemoryOutputStream);


    // STEP 3: [InputStream -> 최종 출력 검증] 2차 스트리밍
    for (size_t i = 0; i < intermediateSize; i++) {
        uint8_t byte;
        bool success = tryNextByte_BufferedInputStream(bis, &byte, err); // 규칙 반영 접두사 가상 매핑
        ASSERT_EQ(success, true);
        success = tryWriteByte_BufferedOutputStream(bos, byte, err);
        ASSERT_EQ(success, true);
    }

    flush_BufferedOutputStream(bos, err);
    ASSERT_EQ(peekSurfaceError_ErrorContext(err), nullptr);

    // 최종 결과물 버퍼 추출 (소유권 영구 탈취)
    size_t finalSize = 0;
    const uint8_t* const finalBytes = borrowInternalBuf_MemoryOutputStream(mos, &finalSize);
    

    // 4. 최종 무결성 데이터 단언 (Assertion)
    ASSERT_EQ(dataSize, finalSize);
    for (size_t i = 0; i < finalSize; i++) {
        // 원본 난수 데이터가 두 번의 스트림 스위칭 및 리셋을 거쳐도 완벽히 유지되었는지 확인
        ASSERT_EQ(srcData[i], finalBytes[i]) << "데이터 손상 위치: 인덱스 " << i;
    }


    // 5. 자원 정리 (수명 주기 해제)
    destroy_ErrorContext(err);
    destroy_MemoryInputStream(mis);
    destroy_MemoryOutputStream(mos);
    destroy_BufferedInputStream(bis);
    destroy_BufferedOutputStream(bos);
}

TEST(BufferedStreamUnit, ShouldReadUntilLimit) {
    // 1. 순수 단위 테스트용 데이터셋 준비 (1KB 원본 데이터)
    size_t dataSize = 1024;
    size_t limit = 512;

    std::vector<uint8_t> srcData(dataSize);

    std::mt19937 gen(42); // 재현 가능성을 위해 시드 고정
    std::uniform_int_distribution<int> dis(0, 255);
    for (size_t i = 0; i < dataSize; i++) {
        srcData[i] = dis(gen);
    }

    ErrorContext* err = create_ErrorContext();

    // 2. 최하부 물리 메모리 스트림 생성 (소유권 관리 대상)

    MemoryInputStream* mis = create_MemoryInputStream();
    ASSERT_NE(mis, nullptr);

    fill_MemeoryInputStream(mis, (uint8_t*)srcData.data(), dataSize);

    // 3. 상위 버퍼 스트림 객체 생성 및 래핑
    BufferedInputStream* bis = create_BufferedInputStream(mis, fetch_MemoryInputStream);
    ASSERT_NE(bis, nullptr);
    setLimit_BufferedInputStream(bis, limit);


    uint8_t byte;
    size_t cnt = 0;

    while (tryNextByte_BufferedInputStream(bis, &byte, err)) {
        ASSERT_EQ(byte, srcData[cnt]);
        cnt++;
    }

    ASSERT_EQ(cnt, limit);

    clearLimit_BufferedInputStream(bis);
    consumeSurfaceError_ErrorContext(err);

    bool success = tryNextByte_BufferedInputStream(bis, &byte, err);
    ASSERT_EQ(success, true);
    ASSERT_EQ(byte, srcData[limit]);

    // 5. 자원 정리 (수명 주기 해제)
    destroy_ErrorContext(err);
    destroy_MemoryInputStream(mis);
    destroy_BufferedInputStream(bis);
}





