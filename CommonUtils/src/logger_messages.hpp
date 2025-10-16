#pragma once
#include "logger.hpp"

/* 메시지 출력 관련 로그 구현부 */
/* Error Macro */
#define LOG_ERROR_KNOWN_EXCEPTION(msg) logger::messages::logKnownExceptionError(msg)
#define LOG_ERROR_UNKNOWN_EXCEPTION() logger::messages::logUnknownExceptionError()

/* Message Macro */
#define LOG_MESSAGE_ENTER_PRICING() logger::messages::enterPricing()
#define LOG_MESSAGE_COUPON_SCHEDULE_GENERATE() logger::messages::couponScheduleGenerate()
#define LOG_MESSAGE_COUPON_SCHEDULE_INPUT() logger::messages::couponScheduleInput()

/* Message / Error Log Function */
namespace logger::messages {
    inline void couponScheduleGenerate() {
        logger::info("Coupon Schedule Data Not Received, Generating Schedule.");
    }

    inline void couponScheduleInput() {
        logger::info("Coupon Schedule Data Received, Using Input Data.");
    }

    inline void enterPricing() {
        info("Pricing Process Start.");
        info("");
    }

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
