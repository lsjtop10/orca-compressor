#include <gtest/gtest.h>

extern "C" {
#include "errors.h"
}

TEST(ErrorContext, shouldCreateAndAppendError) {
    ErrorContext* errCtx = create_ErrorContext();
    ASSERT_NE(errCtx, nullptr);

    append_ErrorContext(errCtx, HF_ERR_MEMALLOC_FAILED, "Memory allocation failed");
    append_ErrorContext(errCtx, HF_ERR_STREAM_BURST, "Stream burst error");
    //
    ASSERT_TRUE(contains_ErrorContext(errCtx, HF_ERR_MEMALLOC_FAILED));
    ASSERT_TRUE(contains_ErrorContext(errCtx, HF_ERR_STREAM_BURST));

    destroy_ErrorContext(errCtx);
}

TEST(ErrorContext, shouldHandleMultipleErrors) {
    ErrorContext* errCtx = create_ErrorContext();
    ASSERT_NE(errCtx, nullptr);

    append_ErrorContext(errCtx, HF_ERR_MEMALLOC_FAILED, "Memory allocation failed");
    append_ErrorContext(errCtx, HF_ERR_STREAM_BURST, "Stream burst error");

    ASSERT_NE(peekSurfaceError_ErrorContext(errCtx), nullptr);
    ASSERT_TRUE(contains_ErrorContext(errCtx, HF_ERR_MEMALLOC_FAILED));
    ASSERT_TRUE(contains_ErrorContext(errCtx, HF_ERR_STREAM_BURST));

    Error* err = peekSurfaceError_ErrorContext(errCtx);
    ASSERT_NE(err, nullptr);
    ASSERT_EQ(getCode_Error(err), HF_ERR_STREAM_BURST);
    ASSERT_STREQ(getMsg_Error(err), "Stream burst error");

    destroy_ErrorContext(errCtx);
}

TEST(ErrorContext, ShouldPrintErrors) {
    ErrorContext* errCtx = create_ErrorContext();
    ASSERT_NE(errCtx, nullptr);


    append_ErrorContext(errCtx, HF_ERR_MEMALLOC_FAILED, "Memory allocation failed");
    append_ErrorContext(errCtx, HF_ERR_STREAM_BURST, "Stream burst error");

    
    printf("Menual Test:\n");
    printf("Printing errors: Expected\n");
    printf("Memory allocation failed, code: %d s\n", HF_ERR_MEMALLOC_FAILED);
    printf("Stream burst error, code: %d s\n", HF_ERR_STREAM_BURST);

    printf("\n");
    printf("Printing errors: Actual\n");
    printStd_ErrorContext(errCtx);

    destroy_ErrorContext(errCtx);
}