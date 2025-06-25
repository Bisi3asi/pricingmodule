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
#include "ql/currency.hpp"
#include "ql/currencies/all.hpp"
#include "ql/time/calendars/southkorea.hpp"
#include "ql/time/schedule.hpp"
#include "ql/time/daycounters/actualactual.hpp"
#include "ql/termstructures/yield/piecewisezerospreadedtermstructure.hpp"
#include "ql/termstructures/yield/zerocurve.hpp"
#include "ql/quotes/simplequote.hpp"
#include "ql/indexes/iborindex.hpp"
#include "ql/instruments/bonds/zerocouponbond.hpp"
#include "ql/instruments/bonds/fixedratebond.hpp"
#include "ql/instruments/bonds/floatingratebond.hpp"
#include "ql/pricingengines/bond/discountingbondengine.hpp"
#include "ql/cashflows/cashflows.hpp"


// dll export method (extern "C", EXPORT 명시 필요)
extern "C" double EXPORT pricingFRB(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int settlementDays              // INPUT 2. 결제일 offset
    , const int issueDate                   // INPUT 3. 발행일 (serial number)
    , const int maturityDate                // INPUT 4. 만기일 (serial number)
    , const double notional                 // INPUT 5. 채권 원금
    , const double couponRate               // INPUT 6. 쿠폰 이율
    , const int couponDayCounter            // INPUT 7. DayCounter code (TODO)
    , const int couponCalendar              // INPUT 8. Calendar code (TODO)
    , const int couponFrequency             // INPUT 9. Frequency code (TODO)

    , const int numberOfCoupons             // INPUT 10. 쿠폰 개수
    , const int* paymentDates               // INPUT 11. 지급일 배열
    , const int* realStartDates             // INPUT 12. 각 구간 시작일
    , const int* realEndDates               // INPUT 13. 각 구간 종료일

    , const int numberOfGirrTenors          // INPUT 14. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 15. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 16. GIRR 금리

    , const int girrDayCounter              // INPUT 17. GIRR DayCountern (TODO)
    , const int girrInterpolator            // INPUT 18. 보간법 (TODO)
    , const int girrCompounding             // INPUT 19. 이자 계산 방식 (TODO)
    , const int girrFrequency               // INPUT 20. 이자 빈도 (TODO)

    , const double spreadOverYield          // INPUT 21. 채권의 종목 Credit Spread
    , const int spreadOverYieldCompounding  // INPUT 22. 이자 계산 방식 (TODO)
    , const int spreadOverYieldDayCounter   // INPUT 23. DCB (TODO)

    , const int numberOfCsrTenors           // INPUT 24. CSR 만기 수
    , const int* csrTenorDays               // INPUT 25. CSR 만기 (startDate로부터의 일수)
    , const double* csrRates                // INPUT 26. CSR 스프레드 (금리 차이)

    , const double marketPrice              // INPUT 27. 시장가격(Spread Over Yield 산출 시 사용)
    , const double csrRiskWeight            // INPUT 28. csr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용)

    , const int calType			            // INPUT 29. 계산 타입 (1: Price, 2. BASEL 2 민감도, 3. BASEL 3 민감도, 9: SOY)
    , const int logYn                       // INPUT 30. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultBasel2                  // OUTPUT 2. Basel 2 Result(Delta, Gamma, Duration, Convexity, PV01)
    , double* resultGirrDelta               // OUTPUT 3. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultCsrDelta			    // OUTPUT 4. CSR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultGirrCvr			        // OUTPUT 5. GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCsrCvr			        // OUTPUT 6. CSR Curvature [BumpUp Curvature, BumpDownCurvature]
    // ===================================================================================================

);

extern "C" double EXPORT pricingFRN(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int settlementDays              // INPUT 2. 결제일 offset
    , const int issueDate                   // INPUT 3. 발행일 (serial number)
    , const int maturityDate                // INPUT 4. 만기일 (serial number)
    , const double notional                 // INPUT 5. 채권 원금
    , const int couponDayCounter            // INPUT 6. DayCounter code (TODO)

    , const int referenceIndex              // INPUT 7. 참고 금리
    , const int fixingDays                  // INPUT 8. 금리 확정일 수
    , const double gearing                  // INPUT 9. 참여율
    , const double spread                   // INPUT 10. 스프레드
    , const double lastResetRate            // INPUT 11. 직전 확정 금리
    , const double nextResetRate            // INPUT 12. 차기 확정 금리

    , const int numberOfCoupons             // INPUT 13. 쿠폰 개수
    , const int* paymentDates               // INPUT 14. 지급일 배열
    , const int* realStartDates             // INPUT 15. 각 구간 시작일
    , const int* realEndDates               // INPUT 16. 각 구간 종료일

    , const double spreadOverYield          // INPUT 17. 채권의 종목 Credit Spread
    , const int spreadOverYieldCompounding  // INPUT 18. 이자 계산 방식 (TODO)
    , const int spreadOverYieldDayCounter   // INPUT 19. DCB (TODO)

    , const int numberOfGirrTenors          // INPUT 20. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 21. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 22. GIRR 금리

    , const int girrDayCounter              // INPUT 23. GIRR DayCountern (TODO)
    , const int girrInterpolator            // INPUT 24. 보간법 (TODO)
    , const int girrCompounding             // INPUT 25. 이자 계산 방식 (TODO)
    , const int girrFrequency               // INPUT 26. 이자 빈도 (TODO)

    , const int numberOfCsrTenors           // INPUT 27. CSR 만기 수
    , const int* csrTenorDays               // INPUT 28. CSR 만기 (startDate로부터의 일수)
    , const double* csrRates                // INPUT 29. CSR 스프레드 (금리 차이)

    , const int numberOfIndexGirrTenors     // INPUT 30. GIRR 만기 수
    , const int* indexGirrTenorDays         // INPUT 31. GIRR 만기 (startDate로부터의 일수)
    , const double* indexGirrRates          // INPUT 32. GIRR 금리

    , const int indexGirrDayCounter         // INPUT 33. GIRR DayCountern (TODO)
    , const int indexGirrInterpolator       // INPUT 34. 보간법 (TODO)
    , const int indexGirrCompounding        // INPUT 35. 이자 계산 방식 (TODO)
    , const int indexGirrFrequency          // INPUT 36. 이자 빈도 (TODO)

    , const int indexTenorNumber            // INPUT 37. 금리 인덱스 만기의 길이(1, 2, ..)
    , const int indexTenorUnit              // INPUT 38. 금리 인덱스 만기의 표시 단위(D, M, Y)
    , const int indexFixingDays             // INPUT 39. 금리 인덱스의 고시 확정일 수
    , const int indexCurrency               // INPUT 40. 금리 인덱스의 표시 통화
    , const int indexCalendar               // INPUT 41. 금리 인덱스의 휴일 기준 달력
    , const int indexBDC                    // INPUT 42. 금리 인덱스의 휴일 적용 기준
    , const int indexEOM                    // INPUT 43. 금리 인덱스의 월말 여부
    , const int indexDayCounter             // INPUT 44. 금리 인덱스의 날짜 계산 기준

    , const int calType			            // INPUT 45. 계산 타입 (1: Price, 2. BASEL 2 Delta, 3. BASEL 3 GIRR / CSR)
    , const int logYn                       // INPUT 46. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultGirrDelta               // OUTPUT 2. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultIndexGirrDelta          // OUTPUT 3. IndexGIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultCsrDelta			    // OUTPUT 4. CSR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    // ===================================================================================================
);


/* FOR UTIL */
void initResult(double* result, const int size);

void processResultArray(std::vector<QuantLib::Real> tenors, std::vector<QuantLib::Real> sensitivities, QuantLib::Size originalSize, double* resultArray);

/* FOR DEBUG */
std::string qDateToString(const QuantLib::Date& date);

void printAllInputDataFRB(
    const int evaluationDate,
    const int settlementDays,
    const int issueDate,
    const int maturityDate,
    const double notional,
    const double couponRate,
    const int couponDayCounter,
    const int couponCalendar,
    const int couponFrequency,
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
    const double spreadOverYield,
    const int spreadOverYieldCompounding,
    const int spreadOverYieldDayCounter,
    const int numberOfCsrTenors,
    const int* csrTenorDays,
    const double* csrRates,
    const double marketPrice,
    const double csrRiskWeight,
    const int calType
);

void printAllOutputDataFRB(
    const double result,
	const double* resultBasel2,
    const double* resultGirrDelta,
    const double* resultCsrDelta,
	const double* resultGirrCvr,
	const double* resultCsrCvr,
    const int calType
);

void printAllInputDataFRN(
    const int evaluationDate,
    const int settlementDays,
    const int issueDate,
    const int maturityDate,
    const double notional,
    const int couponDayCounter,

    const int referenceIndex,
    const int fixingDays,
    const double gearing,
    const double spread,
    const double lastResetRate,
    const double nextResetRate,

    const int numberOfCoupons,
    const int* paymentDates,
    const int* realStartDates,
    const int* realEndDates,

    const double spreadOverYield,
    const int spreadOverYieldCompounding,
    const int spreadOverYieldDayCounter,

    const int numberOfGirrTenors,
    const int* girrTenorDays,
    const double* girrRates,

    const int girrDayCounter,
    const int girrInterpolator,
    const int girrCompounding,
    const int girrFrequency,

    const int numberOfCsrTenors,
    const int* csrTenorDays,
    const double* csrRates,

    const int numberOfIndexGirrTenors,
    const int* indexGirrTenorDays,
    const double* indexGirrRates,

    const int indexGirrDayCounter,
    const int indexGirrInterpolator,
    const int indexGirrCompounding,
    const int indexGirrFrequency,

    const int indexTenorNumber,
    const int indexTenorUnit,
    const int indexFixingDays,
    const int indexCurrency,
    const int indexCalendar,
    const int indexBDC,
    const int indexEOM,
    const int indexDayCounter,

    const int calType
);

void printAllOutputDataFRN(
	const double resultNetPV,
	const double* resultGirrDelta,
	const double* resultIndexGirrDelta,
	const double* resultCsrDelta
);

void printAllData(const QuantLib::Schedule& schedule);

#endif