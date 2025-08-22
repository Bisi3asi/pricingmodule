#include "net.h"
#include "logger.hpp"
#include "common.hpp"

// namespace
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
        info("==============[Net: pricingNET Logging Ended!]==============");
        closeLogger();
    );

    try {
        /* 로거 초기화 */
        disableConsoleLogging(); // 로깅여부 N일시 콘촐 입출력 비활성화
        if (logYn == 1) {
            initLogger("net.log"); // 생성 파일명 지정
        }
		info("==============[Net: pricingNET Logging Started!]==============");
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
            error("[pricingNet]: Unknown exception occurred.");
            return -1;
        }
    }
}