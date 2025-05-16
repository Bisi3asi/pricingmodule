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
    long evaluationDate,            // 평가일 (serial number, 예: 46164)
    long settlementDays,            // 결제일 offset (보통 2일)
    long issueDate,                 // 발행일
    long maturityDate,              // 만기일
    double notional,                // 채권 원금
    double couponRate,              // 쿠폰 이율
    int couponDayCounter,           // DayCounter code (예: 5 = Actual/Actual(Bond))
    int numberOfCoupons,            // 쿠폰 개수
    const long* paymentDates,       // 지급일 배열
    const long* realStartDates,     // 각 구간 시작일
    const long* realEndDates,       // 각 구간 종료일
    int numberOfGirrTenors,         // GIRR 만기 수
    const long* girrTenorDays,      // GIRR 만기 (startDate로부터의 일수)
    const double* girrRates,        // GIRR 금리
    int girrDayCounter,             // GIRR DayCounter (예: 1 = Actual/365)
    int girrInterpolator,           // 보간법 (예: 1 = Linear)
    int girrCompounding,            // 이자 계산 방식 (예: 1 = Continuous)
    int girrFrequency,              // 이자 빈도 (예: 1 = Annual)
    double spreadOverYield,         // 채권의 종목 Credit Spread
    int spreadOverYieldCompounding, // Continuous
    int spreadOverYieldDayCounter,  // Actual/365
    int numberOfCsrTenors,          // CSR 만기 수
    const long* csrTenorDays,       // CSR 만기 (startDate로부터의 일수)
    const double* csrRates,         // CSR 스프레드 (금리 차이)
    
	const int logYn,                // 로깅여부 (0: No, 1: Yes)

                                    // OUTPUT1. Net PV
    double* resultGirrDelta,        // OUTPUT2. GIRR Delta
    double* resultCsrDelta			// OUTPUT3. CSR Delta
);

void initResult(double* result, const int size);

/* for debug */
void printAllInputData(
    long evaluationDate,
    long settlementDays,
    long issueDate,
    long maturityDate,
    double notional,
    double couponRate,
    int couponDayCounter,
    int numberOfCoupons,
    const long* paymentDates,
    const long* realStartDates,
    const long* realEndDates,
    int numberOfGirrTenors,
    const long* girrTenorDays,
    const double* girrRates,
    int girrDayCounter,
    int girrInterpolator,
    int girrCompounding,
    int girrFrequency,
    double spreadOverYield,
    int spreadOverYieldCompounding,
    int spreadOverYieldDayCounter,
    int numberOfCsrTenors,
    const long* csrTenorDays,
    const double* csrRates
);

void printAllOutputData(
    const double resultNetPV,
    const double* resultGirrDelta,
    const double* resultCsrDelta
);

#endif