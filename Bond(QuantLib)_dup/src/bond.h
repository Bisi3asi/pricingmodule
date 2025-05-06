#ifndef BOND_H
#define BOND_H

// function �ܺ� �������̽� export ����
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
        double notional,                    // ä�� ���� ���ݾ�
        long issueDateSerial,               // ������ (serial number)
        long revaluationDateSerial,         // ���� (serial number)
        long maturityDateSerial,            // ������ (serial number)
        long settlementDays,                // ������ offset (���� 2��)
        
        int couponCnt,                      // ���� ����
        double couponRate,                  // ���� ����
        int couponDCB,                      // DayCounter code (��: 5 = Actual/Actual(Bond))
        const long* realStartDatesSerial,   // �� ���� ������
        const long* realEndDatesSerial,     // �� ���� ������
        const long* paymentDatesSerial,     // ������ �迭

        int girrCnt,                        // GIRR ���� ��
        const double* girrRates,            // GIRR �ݸ�
        int girrDCB,                           // GIRR DayCounter (��: 1 = Actual/365)
        // int girrInterpolator,               // ������ (��: 1 = Linear) (����)
        // int girrCompounding,                // ���� ��� ��� (��: 1 = Continuous) (����)
        // int girrFrequency,                  // ���� �� (��: 1 = Annual) (����)

        double spreadOverYield,             // ä���� ���� Credit Spread
        // int spreadOverYieldCompounding,  // Continuous (����)
        int spreadOverYieldDCB,             // Credit Spread Day Count Basis

        int csrCnt,                         // CSR ���� ��
        const double* csrSpreads,           // CSR �������� (�ݸ� ����)

        unsigned short logYn,               // �α� ���� ���� ���� (0: No, 1: Yes)

        double* resultGirrDelta,
        double* resultCsrDelta
        // ===================================================================================================
    );
}

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

QuantLib::DayCounter getDayCounterByDCB(unsigned short dcb, std::vector<QuantLib::DayCounter>& dayCounters);


#endif