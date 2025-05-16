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
#include <iomanip>

// include (QuantLib)
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
#include <ql/time/daycounters/simpledaycounter.hpp>
#include <ql/time/daycounters/business252.hpp>
#include <ql/time/daycounters/one.hpp>
#include <ql/time/daycounters/business252.hpp>
#include <ql/time/daycounter.hpp>

// dll export method
extern "C" {
    double EXPORT_API pricing(
        // ===================================================================================================
        double notional,                    // 채권 원금 명목금액
        long issueDateSerial,               // 발행일 (serial number)
        long maturityDateSerial,            // 만기일 (serial number)
        long revaluationDateSerial,         // 평가일 (serial number)
        long settlementDays,                // 결제일 offset (보통 2일)

        int couponCnt,                      // 쿠폰 개수
        double couponRate,                  // 쿠폰 이율
        int couponDcb,                      // 쿠폰 계산 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        int businessDayConventionCode,          // 영업일 조정 Convention [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3] 
        const long* realStartDatesSerial,   // 각 구간 시작일
        const long* realEndDatesSerial,     // 각 구간 종료일
        const long* paymentDatesSerial,     // 지급일 배열

        int girrCnt,                        // GIRR 만기 수
        const double* girrRates,            // GIRR 금리
        int girrDcb,                        // GIRR 계산 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        int girrInterpolatorCode,               // 보간법 (Linear) (고정)
        int girrCompoundingCode,                // 이자 계산 방식 (Continuous) (고정)
        int girrFrequencyCode,                  // 이자 빈도 (Annual) (고정)

        double spreadOverYield,             // 채권의 종목 Credit Spread
        int spreadOverYieldCompoundingCode,     // Continuous (고정)
        int spreadOverYieldDcb,             // Credit Spread Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        int csrCnt,                         // CSR 만기 수
        const long* csrDates,               // CSR 만기 (startDate로부터의 일수)
        const double* csrSpreads,           // CSR 스프레드 (금리 차이)

        unsigned short logYn,               // 로그 파일 생성 여부 (0: No, 1: Yes)

        // OUTPUT 1. NETPV (리턴값)
        double* resultGirrDelta,            // OUTPUT 2. GIRR 결과값 ([index 0] array size, [index 1 ~ size -1] Girr Delta tenor, [size ~ end] Girr Delta Sensitivity)    
        double* resultCsrDelta  	        // OUTPUT 3. CSR 결과값 ([index 0] array size, [index 1 ~ size -1] Csr Delta tenor, [size ~ end] Csr Delta Sensitivity)   
        // ===================================================================================================
    );
}

void initResult(double* result, int size);

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

void initDayConventions(std::vector<QuantLib::BusinessDayConvention>& dayConventions);

void initFrequencies(std::vector<QuantLib::Frequency>& frequencies);

QuantLib::DayCounter getDayCounterByCode(const int code, const std::vector<QuantLib::DayCounter>& dayCounters);

QuantLib::BusinessDayConvention getBusinessDayConventionByCode(const int code, std::vector<QuantLib::BusinessDayConvention>& dayConventions);

QuantLib::Frequency getFrequencyByCode(const int code, const std::vector<QuantLib::Frequency>& frequencies) {


#endif