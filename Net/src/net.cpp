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

    FINALLY({
        /* Output Result 로그 출력 */
        LOG_OUTPUT(
            FIELD_VAR(result)
        );
        /* 로그 종료 */
        LOG_END(result);
    });

    try {
        /* 로거 초기화 */
        disableConsoleLogging();
        if (logYn == 1) {
            LOG_START("net");
        }

        /* Input Parameter 로그 출력 */
        LOG_INPUT(
            FIELD_VAR(evaluationDate), FIELD_VAR(notional), FIELD_VAR(logYn)
        );
        
        /* 평가 로직 시작 */
        LOG_MESSAGE_ENTER_PRICING();
        return result = notional;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            LOG_ERROR_KNOWN_EXCEPTION(std::string(e.what()));
            return result = -1.0;
        }
        catch (...) {
            LOG_ERROR_UNKNOWN_EXCEPTION();
            return result = -1.0;
        }
    }
}