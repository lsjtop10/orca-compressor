#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include <stdbool.h>
#include <stdio.h>
#include "ownership.h"

typedef enum ErrorCode {
    HF_ERR_UNKOWN,
    HF_STATE_END_OF_STREAM,
    HF_ERR_INVALID_ARG,
    HF_ERR_MEMALLOC_FAILED,
    HF_ERR_STREAM_BURST,
    HF_ERR_FILE_OPEN_FAILED,
    HF_ERR_CONVERT_WCHAR_FAILED,
    HF_ERR_STREAM_WRITE_FAILED,
    HF_ERR_STREAM_READ_FAILED,
    HF_ERR_STREAM_FETCH_FAILED,
    HF_ERR_STREAM_FLUSH_FAILED,
    HF_ERR_EMPTY_HUFF_TREE,
    HF_ERR_CORRUPTED_TREE,

} ErrorCode;

typedef struct Error Error;

ErrorCode getCode_Error(Error* error);
Borrow(char*) getMsg_Error(Error* error);


/**
* @brief 한 번의 작업 단위동안 발생한 에러 상태를 보관하는 구조체
* 
*/
typedef struct ErrorContext ErrorContext;

Move(ErrorContext*) create_ErrorContext(void);

/**
 * @brief 기존 에러를 새로운 컨텍스트로 감쌉니다. 
 * @param err 소유권이 이 에러 객체로 완전히 이동(Move)되는 하위 에러 체인
 * @param msg 추가할 런타임 컨텍스트 문자열 (Move)
 */
void append_ErrorContext(ErrorContext* err, ErrorCode code, Borrow(char*) msg);

/**
 * @brief 
 */
bool contains_ErrorContext(ErrorContext* err, ErrorCode code);


/**
 * @brief check if the error code is the root error code. If no root error exists, returns false. 
 */

bool isRootCode_ErrorContext(ErrorContext* err, ErrorCode code);

/**
 * @brief check if the error code is the surface error code. If no surface error exists, returns false.
 */
bool isSurfaceCode_ErrorContext(ErrorContext* err, ErrorCode code);


/** 
*@brief consume the surface error and return it. If no error exists, returns NULL.
*/
Move(Error*) consumeSurfaceError_ErrorContext(ErrorContext* err);

/** 
*@brief peek the surface error without consuming it. If no error exists, returns NULL.
*/
Error* peekSurfaceError_ErrorContext(ErrorContext* err);

// convert inline fucction


/**
 * @brief 최상단 에러부터 최하단 에러까지 고구마 줄기 캐듯 역추적하여 출력합니다.
 */
void printStd_ErrorContext(ErrorContext* err);


/**
 * @brief print error to standard file. i.e file, socket
 */
void printFile_ErrorContext(ErrorContext* err, FILE* fp);

/**
 * @brief 에러 링크드 리스트 전체와 내부의 모든 동적 msg 문자열을 연쇄 해제합니다.
 */
void destroy_ErrorContext(ErrorContext* err);

#endif