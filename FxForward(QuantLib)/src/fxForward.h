#ifndef FXFORWARD_H
#define FXFORWARD_H

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
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <spdlog/spdlog.h> // LOGGING 
#include <spdlog/sinks/basic_file_sink.h> // LOGGING

// incude (quantlib)
#include <ql/time/date.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/math/interpolations/linearinterpolation.hpp>

// dll export method
extern "C" void EXPORT pricing(
    // ===================================================================================================
    long maturityDate                         // INPUT 1.  만기일 (Maturity Date) 
    , long revaluationDate                      // INPUT 2.  평가일 (Revaluation Date)
    , double exchangeRate                       // INPUT 3.  현물환율 (DOM / FOR)  (Exchange Rate)

    , const char* buySideCurrency               // INPUT 4.  매입 통화 (Buy Side Currency)
    , double notionalForeign                    // INPUT 5.  매입 외화기준 명목금액 (NotionalF)
    , unsigned short buySideDCB                 // INPUT 6.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
    , const char* buySideDcCurve                // INPUT 7.  매입 기준 할인 커브 (Buy Side Discount Curve)

    , unsigned int buyCurveDataSize             // INPUT 8.  매입 커브 데이터 사이즈
    , const unsigned short* buyCurveTerm        // INPUT 9.  매입 커브 만기 기간 (Buy Curve Term)  
    , const unsigned short* buyCurveUnit        // INPUT 10. 매입 커브 만기 기간 단위 (Buy Curve Unit) [Y = 1, M = 2, W = 3, D = 4]
    , const double* buyMarketData               // INPUT 11. 매입 커브 마켓 데이터 (Buy Curve Market Data)

    , const char* sellSideCurrency              // INPUT 12. 매도 통화 (sell Side Currency)
    , double notionalDomestic                   // INPUT 13. 매도 원화기준 명목금액 (NotionalD)
    , unsigned short sellSideDCB                // INPUT 14. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
    , const char* sellSideDcCurve               // INPUT 15. 매도 기준 할인 커브 (Buy Side Discount Curve)

    , unsigned int sellCurveDataSize            // INPUT 16. 매도 커브 데이터 사이즈
    , const unsigned short* sellCurveTerm       // INPUT 17. 매도 커브 만기 기간 (Sell Curve Term)
    , const unsigned short* sellCurveUnit       // INPUT 18. 매도 커브 만기 기간 단위 (Sell Curve Unit) [Y = 1, M = 2, W = 3, D = 4]
    , const double* sellMarketData              // INPUT 19. 매도 커브 마켓 데이터 (Sell Curve Market Data) 
    , double* result                            // OUTPUT : 결과값 (Net PV, FX sensitivity, GIRR Sensitivity)
    // ===================================================================================================
);

/* struct */
struct TradeInformation {
    QuantLib::Date maturityDate;
    QuantLib::Date revaluationDate;
    QuantLib::Real exchangeRate;
};

struct BuySideValuationCashFlow {
    char currency[4];
    QuantLib::Real principalAmount;
    QuantLib::Date cashFlowDate;
    QuantLib::Real yearFrac;
    unsigned short dcb;
    char dcCurve[20];
    QuantLib::Real dcRate;
    QuantLib::Real dcFactor;
    QuantLib::Real presentValue;
    QuantLib::Real domesticNetPV;
};

struct SellSideValuationCashFlow {
    char currency[4];
    QuantLib::Real principalAmount;
    QuantLib::Date cashFlowDate;
    QuantLib::Real yearFrac;
    unsigned short dcb;
    char dcCurve[20];
    QuantLib::Real dcRate;
    QuantLib::Real dcFactor;
    QuantLib::Real presentValue;
};

struct Curve {
    char curveId[20];
    unsigned short term;
    unsigned short unit;
    unsigned short dcb;
    QuantLib::Real marketData;
    QuantLib::Date maturityDate;
    QuantLib::Real yearFrac;
    QuantLib::Real dcFactor;
    QuantLib::Real zeroRate;
};

struct Girr {
    char irCurveId[20];
    unsigned short term;
    unsigned short unit;
    QuantLib::Real sensitivity;
};


/* function */
void inputTradeInformation(long maturityDateSerial, long revaluationDateSerial, double exchangeRate, TradeInformation& tradeInformation);

void inputBuySideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, unsigned short dcb, const char* dcCurve, BuySideValuationCashFlow& bSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputSellSideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, unsigned short dcb, const char* dcCurve, SellSideValuationCashFlow& sSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputCurveData(
      unsigned int buyCurveDataSize, const char* buySideDcCurve, const unsigned short* buyCurveTerm, const unsigned short* buyCurveUnit, unsigned short buySideDcb, const double* buyMarketData
    , unsigned int sellCurveDataSize, const char* sellSideDcCurve, const unsigned short* sellCurveTerm, const unsigned short* sellCurveUnit, unsigned short sellSideDcb, const double* sellMarketData
    , std::vector<Curve>& curves
);

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

void curveMaturity(long baseDateSerial, std::vector<Curve>& curves);

QuantLib::DayCounter getDayCounterByDCB(unsigned short dcb, std::vector<QuantLib::DayCounter>& dayCounters);

void setYearFrac(QuantLib::Date startDate, std::vector<Curve>& curves, std::vector<QuantLib::DayCounter> dayCounters);

void setDcRate(const char* irCurveId, const short term, const short unit, std::vector<Curve>& curves);

void girrDeltaRiskFactor(const char* buySideCurrency, const char* buySideDcCurve, const char* sellSideCurrency, const char* sellSideDcCurve, std::vector<Girr>& girr);

void bootstrapZeroCurveContinuous(std::vector<Curve>& curves);

void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves);

QuantLib::Real linterp(const std::vector<QuantLib::Real>& yearFractions, const std::vector<QuantLib::Real>& zeroRates, QuantLib::Real targetYearFraction);

QuantLib::Real calNetPV(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const char* buySideCurrency);

QuantLib::Real calFxSensitivity(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const char* buySideCurrency, const QuantLib::Real domesticNetPV);

std::vector<QuantLib::Real> getCurveYearFrac(const char* curveId, const std::vector<Curve>& curves);

std::vector<QuantLib::Real> getCurveZeroRate(const char* curveId, const std::vector<Curve>& curves);

/* FOR DEBUG */
//void printAllData(
//    const TradeInformation& tradeInformation, 
//    const BuySideValuationCashFlow& bSideCashFlow,
//    const SellSideValuationCashFlow& sSideCashFlow,
//    const std::vector<Curve>& curves,
//    const std::vector<Girr>& girrs
//);

#endif