#ifndef BOND_H
#define BOND_H

// function �ܺ� �������̽� export ����
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

// dll export method (extern "C", EXPORT ��� �ʿ�)
extern "C" void EXPORT printSettlementDate (
	/* define function of the parameter here and also .cpp files */
	long tradeDate,
	long settlementDays
);


extern "C" double EXPORT ZeroBondTest(
    long evaluationDate,             // ���� (serial number, ��: 46164)
    long settlementDays,             // ������ offset (���� 2��)
    long issueDate,                  // ������
    long maturityDate,               // ������
    double notional,                 // ä�� ����
    double couponRate,              // ���� ����
    int couponDayCounter,           // DayCounter code (��: 5 = Actual/Actual(Bond))
    int numberOfCoupons,            // ���� ����
    const long* paymentDates,       // ������ �迭
    const long* realStartDates,     // �� ���� ������
    const long* realEndDates,       // �� ���� ������
    int numberOfGirrTenors,         // GIRR ���� ��
    const long* girrTenorDays,      // GIRR ���� (startDate�κ����� �ϼ�)
    const double* girrRates,        // GIRR �ݸ�
    int girrDayCounter,             // GIRR DayCounter (��: 1 = Actual/365)
    int girrInterpolator,           // ������ (��: 1 = Linear)
    int girrCompounding,            // ���� ��� ��� (��: 1 = Continuous)
    int girrFrequency,              // ���� �� (��: 1 = Annual)
    double spreadOverYield,         // ä���� ���� Credit Spread
    int spreadOverYieldCompounding, // Continuous
    int spreadOverYieldDayCounter,  // Actual/365
    int numberOfCsrTenors,          // CSR ���� ��
    const long* csrTenorDays,       // CSR ���� (startDate�κ����� �ϼ�)
    const double* csrSpreads        // CSR �������� (�ݸ� ����)

);



#endif