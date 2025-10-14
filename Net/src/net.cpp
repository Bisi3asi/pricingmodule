#include "net.h"
#include "logger.hpp"
#include "common.hpp"

using namespace QuantLib;
using namespace std;
using namespace logger;

extern "C" double EXPORT pricingNET(
      const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const double notional                   // INPUT 2. 채권 원금
	, const int logYn 					      // INPUT 3. 로깅 여부 (0: No, 1: Yes)
) {
    /* 로거 종료 */
    FINALLY(
        LOG_END();
    );

    try {
        /* 로거 초기화 */
        disableConsoleLogging(); // 기본 콘촐 입출력 비활성화
        if (logYn == 1) {
            LOG_START("net.log");
        }
        
        /* 입력 파라미터 로깅 */
        logNetInput(evaluationDate, notional);
        /* 산출 결과 로깅 */
        logNetOutput(notional);
        
        return notional;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            error("Exception occurred: {}", string(e.what()));
            return -1;
        }
        catch (...) {
            error("[pricingNET]: Unknown exception occurred.");
            return -1;
        }
    }
}

/* 디버깅 관련 */
static void logNetInput(
      const int evaluationDate
    , const double notional
) {
    info("[pricingNET] - Input Parameters");
    LOG_VAR(evaluationDate);
    LOG_VAR(notional);
}

static void logNetOutput(
      const double result
) {
    info("[pricingNET] - Output Results");
    LOG_VAR(result);
}