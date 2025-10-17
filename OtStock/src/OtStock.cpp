//---------------------------------------------------------------
//	모듈명 : stockPricing 처리하기 위한 모듈
//	모듈ID : stockPricing
//---------------------------------------------------------------

#include "otStock.h"
#include "logger_data.hpp"
#include "logger_messages.hpp"
#include "common.hpp"

using namespace QuantLib;
using namespace std;
using namespace logger;

extern "C" double EXPORT pricing(
	const double amount				// INPUT 1. 현재가치금액 - SPOT_PRICE
	, const double price            // INPUT 2. 종가 (시나리오 분석시 시나리오 적용가 )  - 커브적용 분석시 종가  CURVE_EQ_SPOT        : IC-KOSPI200
	, const double basePrice        // INPUT 3. 기준가		- BOOK_PRICE
	, const double beta             // INPUT 4. 지수기준 이용시 종목 Beta값
	, const int calType             // INPUT 5. (1:pric, 2:basel2 sensitivity, 3:basel3 sensitivity, 4:cashflow)
	, const int scenCalcu           // INPUT 6. 일반(이론가) : 0 , 일반시나리오분석작업 : 1 , RM시나리오분석 : 2
	, const int logYn               // INPUT 7. (0:No, 1:Yes)
									// OUTPUT 1. 이론가
	, double* resultBasel2          // OUTPUT 2. (basel2 sensitivity, Delta, Gamma, Vega, duration, convexitym pv01 )
	, double* resultBasel3			// OUTPUT 3. (basel3 sensitivity, GIRR, CSR-nSec, CSR-nCTP, CSR-CTP, EQ, CM, FX ) 
	, double* resultCashflow		// OUTPUT 4. (Cashflow)  -
) {
	/* TODO */
	// 1. 수수료, 세금, 2영업일 지급 등 검토
	double result = -1.0; // 결과값 리턴 변수
	
	FINALLY({
		/* Output Result 로그 출력 */
		LOG_OUTPUT(
			FIELD_VAR(result), 
			FIELD_ARR(resultBasel2, 6), FIELD_ARR(resultBasel3, 1), FIELD_ARR(resultCashflow, 1)
		);
		/* 로그 종료 */
		LOG_END(result);
	});
	try {
		/* 로거 초기화 */
		disableConsoleLogging();
		if (logYn == 1) {
			LOG_START("otStock");
		}

		/* Input Parameter 로그 출력 */
		LOG_INPUT(
			FIELD_VAR(amount), FIELD_VAR(price), FIELD_VAR(basePrice), FIELD_VAR(beta), 
			FIELD_VAR(calType), FIELD_VAR(scenCalcu), FIELD_VAR(logYn)
		);

		initResult(resultBasel2, 6);
		initResult(resultBasel3, 1);
		initResult(resultCashflow, 1);

		long		retNo		= 0 ;		// return cole
		double      npv			= 0 ;		// 평가 금액
		double      retDeta     = 0 ;		
		// amt * ( price / basePrice ) ^ beta

		/* 평가 로직 시작 */
		LOG_MSG_PRICING_START();

		if (scenCalcu == 1 ) {
			LOG_MSG_PRICING("Net PV - Normal Scenario Analysis");
			npv =  amount * pow( (price / basePrice) , beta );
		}
		else if (scenCalcu == 2) {
			LOG_MSG_PRICING("Net PV - RM Scenario Analysis");
			npv = amount * (price / basePrice);
		}
		else {
			LOG_MSG_PRICING("Net PV - No Scenario Analysis");
		    npv = amount;
		}

		/* 결과 로드 */
		LOG_MSG_LOAD_RESULT("Net PV");

		// basel2 sensitivity
		if (calType == 2) {
			LOG_MSG_PRICING("Basel 2 Sensitivity")
			LOG_MSG_PRICING("Basel 2 Sensitivity - Delta");
			LOG_MSG_LOAD_RESULT("Basel 2 Sensitivity - Delta");

			resultBasel2[0] = npv;	// Delta
			resultBasel2[1] = 0;	// Gamma
			resultBasel2[2] = 0;	// Vega
			resultBasel2[3] = 0;	// Theta
			resultBasel2[4] = 0;	// Rho
			resultBasel2[5] = 0;	// PV01 (TODO)
		}

		// basel3 sensitivity
		if (calType == 3) {
			LOG_MSG_PRICING("Basel 3 Sensitivity");
			LOG_MSG_PRICING("Basel 2 Sensitivity - EQ Delta");
			LOG_MSG_LOAD_RESULT("Basel 3 Sensitivity - EQ Delta");

			resultBasel3[0] = npv;
		}

		if (calType == 4) {
		}

		if (retNo != 0) {
			return result = retNo;
		}
		else {
			return result = npv;
		}

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


