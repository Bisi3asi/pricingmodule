#ifndef FXFORWARD_H
#define FXFORWARD_H

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
        double notional                             // INPUT 1.  명목금액
        , double exchangeRate                       // INPUT 2.  환율
        , long issueDateSerial                      // INPUT 3.  채권 발행일 (serial) 
        , long maturityDateSerial                   // INPUT 4.  채권 만기일 (serial)
        , long revaluationDateSerial                // INPUT 5.  채권 평가기준일 (serial)
        , double couponRate                         // INPUT 6.  쿠폰 금리
        , int couponFrequencyMonth                  // INPUT 7.  쿠폰 지급 주기 (month)
        , int couponDcb                             // INPUT 8.  쿠폰 지급일자 산출간 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , int accuralDcb                            // INPUT 9.  쿠폰 가격 산출간 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , int businessDayConvention                 // INPUT 10.  영업일 계산방법 [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3]
        , int periodEndDateConvention               // INPUT 11.  기간 월말 계산방법 [Adjusted = 0, UnAdjusted = 1]

        , const char* dcCurveId                     // INPUT 12. 할인 커브 ID
        , int dcCurveDataSize                       // INPUT 13. 할인 커브 데이터 개수  
        , const double* dcCurveYearFrac             // INPUT 14. 할인 커브 연도분수
        , const double* dcCurveMarketData           // INPUT 15. 할인 커브 할인율

        // , const char* crsCurveId                    // INPUT 16. CRS 커브 ID 
        // , int crsCurveDataSize                      // INPUT 17. CRS 커브 데이터 개수
        // , const double* crsCurveYearFrac            // INPUT 18. CRS 커브 연도분수
        // , const double* crsCurveMarketData          // INPUT 19. CRS 커브 할인율

        , int logYn                                 // INPUT 20. 로그 파일 생성 여부 (0: No, 1: Yes)

                                                    // OUTPUT 1. 결과값 (NET PV, Return Value)
        , double* resultGirrDelta 			        // OUTPUT 2. 결과값 ([index 0] array size, [index 1 ~ size] Girr Delta tenor, [size + 1 ~ end] Girr Delta Sensitivity)
        // , double* resultCsrDelta                         // OUTPUT 3. 결과값 ([index 0] array size, [index 1 ~ size] Csr Delta tenor, [size + 1 ~ end] Csr Delta Sensitivity)
        // ===================================================================================================
    );
}

/* struct */
struct TradeInformation {
    QuantLib::Real notional;
    QuantLib::Real exchangeRate;
    QuantLib::Date issueDate;
    QuantLib::Date maturityDate;
    QuantLib::Date revaluationDate;
    QuantLib::Real couponRate;
    int couponFrequencyMonth;
    // QuantLib::Frequency couponFrequency;
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
    QuantLib::Real coupon;
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
void initResult(double* result, const int size);

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

void initDayConventions(std::vector<QuantLib::BusinessDayConvention>& dayConventions);

void inputTradeInformation(const long notional, const long exchangeRate, const long issueDateSerial, const long maturityDateSerial, const long revaluationDateSerial, const double couponRate, const double couponFrequencyMonth, const int couponDcb, const int accuralDcb, const int businessDayConvention, const int periodEndDateConvention, const char* dcCurveId, /* const char* crsCurveId, */ TradeInformation& tradeInfo, /* std::vector<QuantLib::Frequency> frequencies, */ const std::vector<QuantLib::DayCounter>& dayCounters, const std::vector<QuantLib::BusinessDayConvention>& dayConventions);

void inputCouponCashFlow(const TradeInformation& tradeInfo, std::vector<CouponCashflow>& cashflows);

void inputCurveData(const char* curveId, int curveDataSize, const double* yearFrac, const double* marketData, std::vector<Curve>& curves);

void inputGirrDelta(const char* curveId, const double maxYearFrac, std::vector<Girr>& girrs, double* resultGIRRDelta);

void setCurveDcRate(const Girr& girr, std::vector<Curve>& curves, const QuantLib::Real dcRate);

void bootstrapZeroCurveContinuous(std::vector<Curve>& curves);

void discountBond(const TradeInformation& tradeInfo, const std::vector<Curve>& curves, std::vector<CouponCashflow>& cashflows);

QuantLib::Real linterp(const std::vector<QuantLib::Real>& yearFractions, const std::vector<QuantLib::Real>& zeroRates, QuantLib::Real targetYearFraction);

QuantLib::Real processNetPV(const TradeInformation& tradeInfo, std::vector<CouponCashflow>& cashflows, std::vector<Curve>& curves);

QuantLib::Real calNetPV(const std::vector<CouponCashflow>& cashflows, const QuantLib::Real exchangeRate);

void processGirrSensitivity(const TradeInformation& tradeInfo, std::vector<CouponCashflow> cashflows, std::vector<Curve>& curves, Girr& girr, double domesticNetPV, double* resultGirrDelta /*, double* resultCsrDelta */);

QuantLib::Real calGirrSensitivity(const QuantLib::Real domesticNetPV, const QuantLib::Real oroginalNetPV);

std::vector<QuantLib::Real> getCurveYearFrac(const char* curveId, const std::vector<Curve>& curves);

std::vector<QuantLib::Real> getCurveZeroRate(const char* curveId, const std::vector<Curve>& curves);

QuantLib::DayCounter getDayConterByCode(const int dcb, const std::vector<QuantLib::DayCounter>& dayCounters);

QuantLib::BusinessDayConvention getDayConventionByCode(const int businessDayConvention, const std::vector<QuantLib::BusinessDayConvention>& dayConventions);

double roundToDecimals(const double value, const int n);

/* FOR DEBUG */
std::string qDateToString(const QuantLib::Date& date);

void printAllInputData(const double notional, const double exchangeRate, const long issueDateSerial, const long maturityDateSerial, const long revaluationDateSerial, const double couponRate, const int couponFrequencyMonth, const int couponDcb, const int accuralDcb, const int businessDayConvention, const int periodEndDateConvention, const char* dcCurveId, const int dcCurveDataSize, const double* dcCurveYearFrac, const double* dcCurveMarketData);

void printAllOutputData(const double netPV, const double* resultGirrDelta);

void printAllData(const TradeInformation& tradeInfo, const std::vector<CouponCashflow>& cashflows, const std::vector<Curve>& curves, const std::vector<Girr>& girrs);

void printAllData(const TradeInformation& tradeInformation);

void printAllData(const std::vector<CouponCashflow>& cashflows);

void printAllData(const std::vector<Curve>& curves);

void printAllData(const std::vector<Girr>& girrs);

/* TODO */
//void initFrequencies(std::vector<QuantLib::Frequency>& frequencies);

//QuantLib::Frequency getFrequencyByMonth(int month, std::vector<QuantLib::Frequency>& frequencies);

#endif