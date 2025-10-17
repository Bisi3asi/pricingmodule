#pragma once
#include "logger.hpp"

/* 메시지 출력 관련 로그 구현부 */
/* Error Macro */
#define LOG_ERR_KNOWN_EXCEPTION(msg) logger::messages::logKnownExceptionError(__FILE__, __LINE__, (msg))
#define LOG_ERR_UNKNOWN_EXCEPTION() logger::messages::logUnknownExceptionError(__FILE__, __LINE__)

/* Message Macro */
// 해당 로깅 문장을 파일명/라인번호와 함께 출력
#define LOG_MSG(...) logger::infoWithLine(__FILE__, __LINE__, __VA_ARGS__)
// 평가 프로세스 이전 입력 파라미터 검증 시작
#define LOG_MSG_INPUT_VALIDATION() LOG_MSG("Validating Input Parameters.")
// 전체 평가 프로세스 시작
#define LOG_MSG_PRICING_START() LOG_MSG("Pricing Process Start.")
// 평가 <name> 시작
#define LOG_MSG_PRICING(name) LOG_MSG("Pricing {}.", name)
// 쿠폰 스케쥴 데이터 미수신, 스케쥴 생성 중
#define LOG_MSG_COUPON_SCHEDULE_GENERATE() LOG_MSG("Coupon Schedule Data Not Received, Generating Schedule.")
// 쿠폰 스케쥴 데이터 수신, 입력 데이터 사용 중
#define LOG_MSG_COUPON_SCHEDULE_INPUT() LOG_MSG("Coupon Schedule Data Received, Using Input Data.")
// 평가 완료, <name> 결과 로드 중
#define LOG_MSG_LOAD_RESULT(name) LOG_MSG("Loading {} Result to Output.", name)

/* Message / Error Log Function */
namespace logger::messages {
    inline void logKnownExceptionError(const std::string& msg) {
        error("! Exception Occured !");
        error("---------------------------------------------");
        error("{}", msg);
        error("---------------------------------------------");
        info("");
    }

    inline void logUnknownExceptionError() {
        error("! Exception Occured !");
        error("---------------------------------------------");
        error("Unknown Exception, please check the input parameters and environment.");
        error("---------------------------------------------");
        info("");
    }
}
