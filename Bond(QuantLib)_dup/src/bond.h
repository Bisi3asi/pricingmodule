#ifndef BOND_H
#define BOND_H

// function 외부 인터페이스 export 정의
#ifdef _WIN32
    #ifdef BUILD_LIBRARY
        #define EXPORT_API __declspec(dllexport) __stdcall
    #else
        #define EXPORT_API __declspec(dllimport) __stdcall
    #endif
#else
    #define EXPORT_API
#endif

#pragma once

// include (cpp)
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
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/daycounter.hpp>

// dll export method
extern "C" {
    double EXPORT_API pricing(
        // ===================================================================================================
        double notional,                    // 채권 원금 명목금액
        long issueDateSerial,               // 발행일 (serial number)
        long revaluationDateSerial,         // 평가일 (serial number)
        long maturityDateSerial,            // 만기일 (serial number)
        long settlementDays,                // 결제일 offset (보통 2일)
        
        int couponCnt,                      // 쿠폰 개수
        double couponRate,                  // 쿠폰 이율
        int couponDCB,                      // DayCounter code (예: 5 = Actual/Actual(Bond))
        const long* realStartDatesSerial,   // 각 구간 시작일
        const long* realEndDatesSerial,     // 각 구간 종료일
        const long* paymentDatesSerial,     // 지급일 배열

        int girrCnt,                        // GIRR 만기 수
        const double* girrRates,            // GIRR 금리
        int girrDCB,                           // GIRR DayCounter (예: 1 = Actual/365)
        // int girrInterpolator,               // 보간법 (예: 1 = Linear) (고정)
        // int girrCompounding,                // 이자 계산 방식 (예: 1 = Continuous) (고정)
        // int girrFrequency,                  // 이자 빈도 (예: 1 = Annual) (고정)

        double spreadOverYield,             // 채권의 종목 Credit Spread
        // int spreadOverYieldCompounding,  // Continuous (고정)
        int spreadOverYieldDCB,             // Credit Spread Day Count Basis

        int csrCnt,                         // CSR 만기 수
        const double* csrSpreads,           // CSR 스프레드 (금리 차이)

        unsigned short logYn,               // 로그 파일 생성 여부 (0: No, 1: Yes)

        double* resultGirrDelta,
        double* resultCsrDelta
        // ===================================================================================================
    );
}

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

QuantLib::DayCounter getDayCounterByDCB(unsigned short dcb, std::vector<QuantLib::DayCounter>& dayCounters);


#endif