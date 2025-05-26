#ifndef BOND_H
#define BOND_H

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

// include
#include <iostream>
#include <iomanip>

// include (QuantLib)
#include "ql/time/calendars/southkorea.hpp"
#include "ql/termstructures/yield/piecewisezerospreadedtermstructure.hpp"
#include "ql/termstructures/yield/zerocurve.hpp"
#include "ql/quotes/simplequote.hpp"
#include "ql/pricingengines/bond/discountingbondengine.hpp"
#include "ql/instruments/bonds/zerocouponbond.hpp"
#include "ql/instruments/bonds/fixedratebond.hpp"
#include "ql/time/schedule.hpp"
#include "ql/time/daycounters/actualactual.hpp"

// dll export method (extern "C", EXPORT 명시 필요)
extern "C" double EXPORT pricing(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int settlementDays              // INPUT 2. 결제일 offset
    , const int issueDate                   // INPUT 3. 발행일 (serial number)
    , const int maturityDate                // INPUT 4. 만기일 (serial number)
    , const double notional                 // INPUT 5. 채권 원금
    , const double couponRate               // INPUT 6. 쿠폰 이율
    , const int couponDayCounter            // INPUT 7. DayCounter code (TODO)

    , const int numberOfCoupons             // INPUT 8. 쿠폰 개수
    , const int* paymentDates               // INPUT 9. 지급일 배열
    , const int* realStartDates             // INPUT 10. 각 구간 시작일
    , const int* realEndDates               // INPUT 11. 각 구간 종료일

    , const int numberOfGirrTenors          // INPUT 12. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 13. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 14. GIRR 금리

    , const int girrDayCounter              // INPUT 15. GIRR DayCountern (TODO)
    , const int girrInterpolator            // INPUT 16. 보간법 (TODO)
    , const int girrCompounding             // INPUT 17. 이자 계산 방식 (TODO)
    , const int girrFrequency               // INPUT 18. 이자 빈도 (TODO)

    , const double spreadOverYield          // INPUT 19. 채권의 종목 Credit Spread
    , const int spreadOverYieldCompounding  // INPUT 20. 이자 계산 방식 (TODO)
    , const int spreadOverYieldDayCounter   // INPUT 21. DCB (TODO)

    , const int numberOfCsrTenors           // INPUT 22. CSR 만기 수
    , const int* csrTenorDays               // INPUT 23. CSR 만기 (startDate로부터의 일수)
    , const double* csrRates                // INPUT 24. CSR 스프레드 (금리 차이)

    , const int calType			            // INPUT 25. 계산 타입 (1: Price, 2. BASEL 2 Delta, 3. BASEL 3 GIRR / CSR)    
    , const int logYn                       // INPUT 26. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultGirrDelta               // OUTPUT 2. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultCsrDelta			    // OUTPUT 3. CSR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    // ===================================================================================================
);

void initResult(double* result, const int size);

/* FOR DEBUG */
std::string qDateToString(const QuantLib::Date& date);

void printAllInputData(
    const int evaluationDate,
    const int settlementDays,
    const int issueDate,
    const int maturityDate,
    const double notional,
    const double couponRate,
    const int couponDayCounter,
    const int numberOfCoupons,
    const int* paymentDates,
    const int* realStartDates,
    const int* realEndDates,
    const int numberOfGirrTenors,
    const int* girrTenorDays,
    const double* girrRates,
    const int girrDayCounter,
    const int girrInterpolator,
    const int girrCompounding,
    const int girrFrequency,
    double spreadOverYield,
    const int spreadOverYieldCompounding,
    const int spreadOverYieldDayCounter,
    const int numberOfCsrTenors,
    const int* csrTenorDays,
    const double* csrRates,
    const int calType
);

void printAllOutputData(
    const double resultNetPV,
    const double* resultGirrDelta,
    const double* resultCsrDelta
);

void printAllData(const QuantLib::Schedule& schedule);

#endif