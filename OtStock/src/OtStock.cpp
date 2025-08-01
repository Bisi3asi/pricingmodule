//---------------------------------------------------------------
//	모듈명 : stockPricing 처리하기 위한 모듈
//	모듈ID : stockPricing
//---------------------------------------------------------------

#include "otStock.h"
#include "common.cpp"

#include <iostream> 
#include <vector>
#include <string>
#include <stdlib.h>  
#include <string.h>
#include <time.h>
#include <math.h>

using namespace std;

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
	// 1. 로깅 로직 도입
	// 2. INPUT 데이터 로깅
	// 3. 수수료, 세금, 2영업일지급등 검토
	
	initResult(resultBasel2, 6);
	initResult(resultBasel3, 1);
	initResult(resultCashflow, 1);

	long		retNo		= 0 ;		// return cole
	double      npv			= 0 ;		// 평가 금액
	double      retDeta     = 0 ;		
	// amt * ( price / basePrice ) ^ beta

	try {

		if (scenCalcu == 1 ) {
			npv =  amount * pow( (price / basePrice) , beta );
		}
		else if (scenCalcu == 2) {
			npv = amount * (price / basePrice);
		}
		else {
		    npv = amount;
		}

	}
	catch (...) {
		cout << "";
		retNo = -999;
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
		return retNo;
	}
	else {
		return npv;
	}
}
