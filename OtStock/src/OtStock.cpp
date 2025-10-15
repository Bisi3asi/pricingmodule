//---------------------------------------------------------------
//	모듈명 : stockPricing 처리하기 위한 모듈
//	모듈ID : stockPricing
//---------------------------------------------------------------

#include "otStock.h"
#include "logger.hpp"
#include "common.hpp"

using namespace QuantLib;
using namespace std;
using namespace logger;

//----------------------------------------------------------------
// stockPricing
// amt          : 주식 시가
// price        : 가격         ( 종목 위험요인 관리 O : 주식가격 ,      종목 위험요인 관리 X : 주가 지수          ) - 시나리오 적용된 가격 
// basePrice	: 기준가격     ( 종목 위험요인 관리 O : 주식 기준가격 , 종목 위험요인 관리 X : 주가지수 기준 가격 ) - 시나리오 적용전 가격 
// beta         : 주식베타     ( 종목 위험요인 관리 O : 1 ,             종목 위험요인 관리 X : 주식 베타          ) 
// price        : 종목가격     ( 일반분석 - 금일가격, 사후검증 : 금일가격 ) - 커브에서 얻어옴
// prodPrice	: 종목기준가격 ( 일반분석 - 금일가격, 사후검증 : 전일가격 ) - 사후검증의 경우는 전일 인스트루먼트정보를 일자만 변경해서 그대로 이용함.
// resultBasel2 : 0:Delta , 1:Gamma, 2:Vega, 3:Theta, 4:Rho, 5:PV01 
// return		: 평가가격(이론가), -999: ERROR
//----------------------------------------------------------------
extern "C" double EXPORT pricing(
	const double amount				// 현재가치금액 - SPOT_PRICE
	, const double price            // 종가 (시나리오 분석시 시나리오 적용가 )  - 커브적용 분석시 종가  CURVE_EQ_SPOT        : IC-KOSPI200
	, const double basePrice        // 기준가		- BOOK_PRIC
	, const double beta             // 지수기준 이용시 종목 Beta값
	, const int calType             // (1:pric, 2:basel2 sensitivity, 3:basel3 sensitivity, 4:cashflow)
	, const int scenCalcu           // 일반(이론가) : 0 , 일반시나리오분석작업 : 1 , RM시나리오분석 : 2
	, const int logYn               // (0:No, 1:Yes)
	, double* resultBasel2          // (basel2 sensitivity, Delta, Gamma, Vega, duration, convexitym pv01 )
	, double* resultBasel3			// (basel3 sensitivity, GIRR, CSR-nSec, CSR-nCTP, CSR-CTP, EQ, CM, FX ) 
	, double* resultCashflow		// (Cashflow)  -
)
{
	/* TODO */
	// 1. 수수료, 세금, 2영업일지급등 검토
	double result = -1.0; // 결과값 리턴 변수
	
	FINALLY(
		logPricingOutput(result, resultBasel2, resultBasel3, resultCashflow);
        LOG_END(result);
    );
	try {
		disableConsoleLogging();
		if (logYn == 1) {
			LOG_START("otStock");
		}
		logPricingInput(amount, price, basePrice, beta, calType, scenCalcu, logYn);

		initResult(resultBasel2, 6);
		initResult(resultBasel3, 1);
		initResult(resultCashflow, 1);

		long		retNo		= 0 ;		// return cole
		double      npv			= 0 ;		// 평가 금액
		double      retDeta     = 0 ;		
		// amt * ( price / basePrice ) ^ beta

		LOG_ENTER_PRICING();
		
		if (scenCalcu == 1 ) {
			npv =  amount * pow( (price / basePrice) , beta );
		}
		else if (scenCalcu == 2) {
			npv = amount * (price / basePrice);
		}
		else {
		    npv = amount;
		}

		// basel2 sensitivity
		if (calType == 2) {
			resultBasel2[0] = npv;	// Delta
			resultBasel2[1] = 0;	// Gamma
			resultBasel2[2] = 0;	// Vega
			resultBasel2[3] = 0;	// Theta
			resultBasel2[4] = 0;	// Rho
			resultBasel2[5] = 0;	// PV01 (TODO)
		}

		// basel3 sensitivity
		if (calType == 3) {
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
static void logPricingInput(
	const double amount
	, const double price
	, const double basePrice
	, const double beta
	, const int calType
	, const int scenCalcu
	, const int logYn
) {
	info("| Input Parameters |");
	info("---------------------------------------------");
	LOG_VAR(amount);
	LOG_VAR(price);
	LOG_VAR(basePrice);
	LOG_VAR(beta);
	LOG_VAR(calType);
	LOG_VAR(scenCalcu);
	info("---------------------------------------------");
	info("");
}

static void logPricingOutput(
	const double result,
	const double* resultBasel2,
	const double* resultBasel3,
	const double* resultCashflow
) {
	info("| Output Results |");
	info("---------------------------------------------");
	LOG_VAR(result);
	// resultBasel2 is an array of 6 elements (precision 6)
	LOG_ARRAY(resultBasel2, 6, 6);
	// resultBasel3 is an array of 1 element
	LOG_ARRAY(resultBasel3, 1);
	// resultCashflow is an array of 1 element
	LOG_ARRAY(resultCashflow, 1);
	info("---------------------------------------------");
	info("");
}


