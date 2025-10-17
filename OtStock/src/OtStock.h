#ifndef OTSTOCK_H
#define OTSTOCK_H

// function 외부 인터페이스 export 정의
#ifdef _WIN32
#ifdef BUILD_LIBRARY
#define EXPORT __declspec(dllexport) __stdcall
#else
#define EXPORT __declspec(dllimport) __stdcall
#endif
#elif defined(__linux__) || defined(__unix__)
#define EXPORT
#endif

#pragma once

/* include */
#include <iostream> 
#include <vector>
#include <string>
#include <stdlib.h>  
#include <string.h>
#include <time.h>
#include <math.h>

/* dll export method(extern "C", EXPORT 명시 필요) */
extern "C" double EXPORT pricing(
	const double amount				// INPUT 1. 현재가치금액 - SPOT_PRICE
	, const double price            // INPUT 2. 종가 (시나리오 분석시 시나리오 적용가 )  - 커브적용 분석시 종가  CURVE_EQ_SPOT        : IC-KOSPI200
	, const double basePrice        // INPUT 3. 기준가 - BOOK_PRICE
	, const double beta             // INPUT 4. 지수기준 이용시 종목 Beta값
	, const int calType             // INPUT 5. (1:pric, 2:basel2 sensitivity, 3:basel3 sensitivity, 4:cashflow)
	, const int scenCalcu           // INPUT 6. 일반(이론가) : 0 , 일반시나리오분석작업 : 1 , RM시나리오분석 : 2
	, const int logYn               // INPUT 7. (0:No, 1:Yes)
									// OUTPUT 1. 이론가
	, double* resultBasel2          // OUTPUT 2. (basel2 sensitivity, Delta, Gamma, Vega, duration, convexitym pv01 )
	, double* resultBasel3			// OUTPUT 3. (basel3 sensitivity, GIRR, CSR-nSec, CSR-nCTP, CSR-CTP, EQ, CM, FX ) 
	, double* resultCashflow		// OUTPUT 4. (Cashflow)  -
);

#endif