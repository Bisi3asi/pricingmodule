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
#include <ql/time/daycounter.hpp>

// dll export method
extern "C" {
    double EXPORT_API pricing(
        // ===================================================================================================
        double notional,                    // ä�� ���� ���ݾ�
        long issueDateSerial,               // ������ (serial number)
        long maturityDateSerial,            // ������ (serial number)
        long revaluationDateSerial,         // ���� (serial number)
        long settlementDays,                // ������ offset (���� 2��)

        int couponCnt,                      // ���� ����
        double couponRate,                  // ���� ����
        int couponDCB,                      // ���� ��� Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        int businessDayConvention,          // ������ ���� Convention [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3] 
        const long* realStartDatesSerial,   // �� ���� ������
        const long* realEndDatesSerial,     // �� ���� ������
        const long* paymentDatesSerial,     // ������ �迭

        int girrCnt,                        // GIRR ���� ��
        const double* girrRates,            // GIRR �ݸ�
        int girrDCB,                        // GIRR ��� Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        // int girrInterpolator,            // ������ (Linear) (����)
        // int girrCompounding,             // ���� ��� ��� (Continuous) (����)
        // int girrFrequency,               // ���� �� (Annual) (����)

        double spreadOverYield,             // ä���� ���� Credit Spread
        // int spreadOverYieldCompounding,  // Continuous (����)
        int spreadOverYieldDCB,             // Credit Spread Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        int csrCnt,                         // CSR ���� ��
        const double* csrSpreads,           // CSR �������� (�ݸ� ����)

        unsigned short logYn,               // �α� ���� ���� ���� (0: No, 1: Yes)

                                            // OUTPUT 1. NETPV (���ϰ�)
        double* resultGirrDelta,            // OUTPUT 2. GIRR ����� ([index 0] array size, [index 1 ~ size -1] Girr Delta tenor, [size ~ end] Girr Delta Sensitivity)    
        double* resultCsrDelta  	        // OUTPUT 3. CSR ����� ([index 0] array size, [index 1 ~ size -1] Csr Delta tenor, [size ~ end] Csr Delta Sensitivity)   
        // ===================================================================================================
    );
}

void initResult(double* result, int size);

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

void initBDCs(std::vector<QuantLib::BusinessDayConvention>& bdcs);

QuantLib::DayCounter getDayCounterByDCB(unsigned int dcb, std::vector<QuantLib::DayCounter>& dayCounters);

QuantLib::BusinessDayConvention getBusinessDayConventionByBDC(unsigned int businessDayConvention, std::vector<QuantLib::BusinessDayConvention>& bdcs);


#endif