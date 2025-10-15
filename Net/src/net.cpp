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
    double result = -1.0; // 결과값 리턴 함수

    FINALLY(
        logPricingNETOutput(result);
        LOG_END(result);
    );

    try {
        disableConsoleLogging();
        if (logYn == 1) {
            LOG_START("net");
        }
        logPricingNETInput(evaluationDate, notional, logYn);
        LOG_ENTER_PRICING();
        return result = notional;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            LOG_KNOWN_EXCEPTION_ERROR(std::string(e.what()));
            return result = -1.0;
        }
        catch (...) {
            LOG_UNKNOWN_EXCEPTION_ERROR();
            return result = -1.0;
        }
    }
}

/* for logging */
static void logPricingNETInput(
      const int evaluationDate
    , const double notional
    , const int logYn
) {
    info("| Input Parameters |");
    info("---------------------------------------------");
    LOG_VAR(evaluationDate);
    LOG_VAR(notional);
    LOG_VAR(logYn);
    info("---------------------------------------------");
    info("");
}

static void logPricingNETOutput(
      const double result
) {
    info("| Output Results |");
    info("---------------------------------------------");
    LOG_VAR(result);
    info("---------------------------------------------");
    info("");
}