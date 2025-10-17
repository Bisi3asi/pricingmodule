#include "net.h"
#include "logger_data.hpp"
#include "logger_messages.hpp"
#include "common.hpp"

using namespace QuantLib;
using namespace std;
using namespace logger;

extern "C" double EXPORT pricingNET(
      const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const double notional                   // INPUT 2. 채권 원금
	, const int logYn 					      // INPUT 3. 로깅 여부 (0: No, 1: Yes)
                                              // OUTPUT 1. Net PV (리턴값)
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
        LOG_MSG_PRICING_START();
        /* 평가 Net PV */
        LOG_MSG_PRICING("Net PV");
        /* 결과 로드 */
        LOG_MSG_LOAD_RESULT("Net PV");

        return result = notional;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            LOG_ERR_KNOWN_EXCEPTION(std::string(e.what()));
            return result = -1.0;
        }
        catch (...) {
            LOG_ERR_UNKNOWN_EXCEPTION();
            return result = -1.0;
        }
    }
}