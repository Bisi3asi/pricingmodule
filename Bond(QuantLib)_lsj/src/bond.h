#ifndef FXFORWARD_H
#define FXFORWARD_H

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

// include (cpp)]
#include <iostream>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <cmath>
#include <array>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

// incude (quantlib)
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/date.hpp>
#include <ql/math/interpolations/linearinterpolation.hpp>
#include <ql/time/calendars/southkorea.hpp>

// dll export method
extern "C" {
    double EXPORT_API pricing(
        // ===================================================================================================
        long notional                               // INPUT 1.  ���ݾ�
        , long issueDateSerial                      // INPUT 1.  ä�� ������ (serial) 
        , long maturityDateSerial                   // INPUT 2.  ä�� ������ (serial)
        , long revaluationDateSerial                // INPUT 3.  ä�� �򰡱����� (serial)
        , double couponRate                         // INPUT 4.  ���� �ݸ�
        , int couponFrequencyMonth                  // INPUT 5.  ���� ���� �ֱ� (month)
        , int couponDcb                             // INPUT 6.  ���� ���� Day Count Basis  [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , int businessDayConvention                 // INPUT 7.  ������ �����            [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3]
        , int periodEndDateConvention               // INPUT 8.  �Ⱓ ���� �����
        , int accuralDcb                            // INPUT 9.  ���� ���� �ϼ� ��� ���   [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const char* dcCurveId                     // INPUT 10. ���� Ŀ�� ID
        , int dcCurveDataSize                       // INPUT 11. ���� Ŀ�� ������ ����  
        , const double* dcCurveYearFrac             // INPUT 12. ���� Ŀ�� �����м�
        , const double* dcCurveMarketData           // INPUT 13. ���� Ŀ�� ������

        // , const char* crsCurveId                    // INPUT 14. CRS Ŀ�� ID 
        // , int crsCurveDataSize                      // INPUT 15. CRS Ŀ�� ������ ����
        // , const double* crsCurveYearFrac            // INPUT 16. CRS Ŀ�� �����м�
        // , const double* crsCurveMarketData          // INPUT 17. CRS Ŀ�� ������

        , int logYn                                 // INPUT 18. �α� ���� ���� ���� (0: No, 1: Yes)

                                                    // OUTPUT 1. ����� (NET PV, ���ϰ�)
        // , double* resultGirrDelta 			        // OUTPUT 2. ����� ([index 0] array size, [index 1 ~ size] Girr Delta tenor, [size + 1 ~ end] Girr Delta Sensitivity)
        // , double* resultCsrDelta                 // OUTPUT 3. ����� ([index 0] array size, [index 1 ~ size] Csr Delta tenor, [size + 1 ~ end] Csr Delta Sensitivity)
        // ===================================================================================================
    );
}

/* struct */
struct TradeInformation {
    long notional;
    QuantLib::Date issueDate;
    QuantLib::Date maturityDate;
    QuantLib::Date revaluationDate;
    QuantLib::Real couponRate;
    QuantLib::Frequency couponFrequency;
    QuantLib::DayCounter couponDcb;
    QuantLib::DayCounter accuralDcb;
    QuantLib::BusinessDayConvention dayConvention;
    int periodEndDateConvention;
    char dcCurveId[20];
    char crsCurveId[20];
};

struct CouponCashflow {
    short type;
    QuantLib::Date startDate;
    QuantLib::Date endDate;
    QuantLib::Date paymentDate;
    long nlyDays;
    long lyDays;
    QuantLib::Real couponAmount;
    QuantLib::Real yearFrac;
    QuantLib::Real dcRate;
    QuantLib::Real dcFactor;
    QuantLib::Real presentValue;
};

struct Curve {
    char curveId[20];
    QuantLib::Real marketData;
    QuantLib::Real yearFrac;
    QuantLib::Real dcFactor;
    QuantLib::Real zeroRate;
};

struct Girr {
    char curveId[20];
    QuantLib::Real yearFrac;
    QuantLib::Real sensitivity;
};

struct Csr {
    char curveId[20];
    QuantLib::Real yearFrac;
    QuantLib::Real sensitivity;
};


/* function */
void initResult(double* result, int size);

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

void initDayConventions(std::vector<QuantLib::BusinessDayConvention>& dayConventions);

void initFrequencies(std::vector<QuantLib::Frequency>& frequencies);

QuantLib::DayCounter getDayConterByCode(int dcb, std::vector<QuantLib::DayCounter>& dayCounters);

QuantLib::BusinessDayConvention getDayConventionByCode(int businessDayConvention, std::vector<QuantLib::BusinessDayConvention>& dayConventions);

QuantLib::Frequency getFrequencyByMonth(int month, std::vector<QuantLib::Frequency>& frequencies);

void inputTradeInformation(long notional, long issueDateSerial, long maturityDateSerial, long revaluationDateSerial, double couponRate, double couponFrequencyMonth, int couponDcb, int accuralDcb, int businessDayConvention, int periodEndDateConvention, const char* dcCurveId, /* const char* crsCurveId, */ TradeInformation& tradeInfo, std::vector<QuantLib::Frequency> frequencies, std::vector<QuantLib::DayCounter> dayCounters, std::vector<QuantLib::BusinessDayConvention> dayConventions);

void inputCouponCashFlow(TradeInformation& tradeInfo, std::vector<CouponCashflow>& cashflows);

int getLeapDays(const QuantLib::Date& start, const QuantLib::Date& end);

//void inputCurveData(unsigned int buyCurveDataSize, const char* buySideDcCurve, const double* buyYearFrac, const double* buyMarketData, unsigned int sellCurveDataSize, const char* sellSideDcCurve,  const double* sellYearFrac, const double* sellMarketData, std::vector<Curve>& curves);

//void setDcRate(Girr& girr, std::vector<Curve>& curves);

//void girrDeltaRiskFactor(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Girr>& girrs, double* resultGIRRDelta);

//void bootstrapZeroCurveContinuous(std::vector<Curve>& curves);

//double processNetPV(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves);

//void processGirrSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves, Girr& girr, double* resultGirrDelta, double* resultNetPvFxSensitivity);

//void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves);

//QuantLib::Real linterp(const std::vector<QuantLib::Real>& yearFractions, const std::vector<QuantLib::Real>& zeroRates, QuantLib::Real targetYearFraction);

//QuantLib::Real calDomesticNetPV(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const char* buySideCurrency);

//QuantLib::Real calFxSensitivity(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const char* buySideCurrency, const QuantLib::Real domesticNetPV);

//QuantLib::Real calGirrSensitivity(const QuantLib::Real domesticNetPV, const QuantLib::Real oroginalNetPV);

//std::vector<QuantLib::Real> getCurveYearFrac(const char* curveId, const std::vector<Curve>& curves);

//std::vector<QuantLib::Real> getCurveZeroRate(const char* curveId, const std::vector<Curve>& curves);

//double roundToDecimals(double value, int n);

/* FOR DEBUG */
std::string qDateToString(const QuantLib::Date& date);

//void printAllInputData(long maturityDate, long revaluationDate, double exchangeRate, const char* buySideCurrency, double notionalForeign, unsigned short buySideDCB, const char* buySideDcCurve, unsigned int buyCurveDataSize, const double* buyCurveYearFrac, const double* buyMarketData, const char* sellSideCurrency, double notionalDomestic, unsigned short sellSideDCB, const char* sellSideDcCurve, unsigned int sellCurveDataSize, const double* sellCurveYearFrac, const double* sellMarketData, unsigned short calType, unsigned short logYn);

//void printAllOutputData(const double* resultNetPvFxSensitivity, const double* resultGirrDelta);

//void printAllData(const TradeInformation& tradeInformation, const BuySideValuationCashFlow& bSideCashFlow, const SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves, const std::vector<Girr>& girrs);

void printAllData(const TradeInformation& tradeInformation);

void printAllData(const std::vector<CouponCashflow>& cashflows);

void printAllData(const std::vector<Curve>& curves);

void printAllData(const std::vector<Girr>& girrs);

#endif