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
#include <ql/time/date.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/math/interpolations/linearinterpolation.hpp>

// dll export method (extern "C", EXPORT 명시 필요)
extern "C" void EXPORT printSettlementDate (
	/* define function of the parameter here and also .cpp files */
	long tradeDate,
	long settlementDays
);


extern "C" double EXPORT ZeroBondTest(
    long evaluationDate,             // 평가일 (serial number, 예: 46164)
    long settlementDays,             // 결제일 offset (보통 2일)
    long issueDate,                  // 발행일
    long maturityDate,               // 만기일
    double notional,                 // 채권 원금
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
    const double* csrSpreads        // CSR 스프레드 (금리 차이)

);



#endif