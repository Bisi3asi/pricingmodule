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

// dll export method
extern "C" {
    void EXPORT_API pricing(
        // ===================================================================================================
        long maturityDate                           // INPUT 1.  만기일 (Maturity Date) 
        , long revaluationDate                      // INPUT 2.  평가일 (Revaluation Date)
        , double exchangeRate                       // INPUT 3.  현물환율 (DOM / FOR)  (Exchange Rate)

        , const char* buySideCurrency               // INPUT 4.  매입 통화 (Buy Side Currency)
        , double notionalForeign                    // INPUT 5.  매입 외화기준 명목금액 (NotionalF)
        , unsigned short buySideDCB                 // INPUT 6.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , const char* buySideDcCurve                // INPUT 7.  매입 기준 할인 커브 (Buy Side Discount Curve)

        , unsigned int buyCurveDataSize             // INPUT 8.  매입 커브 데이터 사이즈
        , const double* buyCurveYearFrac            // INPUT 9.  매입 커브 만기 기간 (Buy Curve Term)  
        , const double* buyMarketData               // INPUT 10. 매입 커브 마켓 데이터 (Buy Curve Market Data)

        , const char* sellSideCurrency              // INPUT 11. 매도 통화 (sell Side Currency)
        , double notionalDomestic                   // INPUT 12. 매도 원화기준 명목금액 (NotionalD)
        , unsigned short sellSideDCB                // INPUT 13. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , const char* sellSideDcCurve               // INPUT 14. 매도 기준 할인 커브 (Buy Side Discount Curve)

        , unsigned int sellCurveDataSize            // INPUT 15. 매도 커브 데이터 사이즈
        , const double* sellCurveYearFrac		    // INPUT 16. 매도 커브 만기 기간 (Sell Curve Term)     
        , const double* sellMarketData              // INPUT 17. 매도 커브 마켓 데이터 (Sell Curve Market Data) 

        , unsigned short calType                    // INPUT 18. 평가 타입 (1: NetPV, 2: GIRR Sensitivity)
        , unsigned short logYn                      // INPUT 19. 로그 파일 생성 여부 (0: No, 1: Yes)

        , double* resultNetPvFxSensitivity          // OUTPUT 1. 결과값 ([index 0] Net PV, [index 1] FX sensitivity)
        , double* resultGirrDelta 			        // OUTPUT 2. 결과값 ([index 0] array size, [index 1 ~ size -1] Girr Delta tenor, [size ~ end] Girr Delta Sensitivity)    
        // ===================================================================================================
    );
}

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
    QuantLib::Real marketData;
    QuantLib::Real yearFrac;
    QuantLib::Real dcFactor;
    QuantLib::Real zeroRate;
};

struct Girr {
    char irCurveId[20];
    QuantLib::Real yearFrac;
    QuantLib::Real sensitivity;
};


/* function */
void initResult(double* result, int size);

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

void inputTradeInformation(long maturityDateSerial, long revaluationDateSerial, double exchangeRate, TradeInformation& tradeInformation);

void inputBuySideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, unsigned short dcb, const char* dcCurve, BuySideValuationCashFlow& bSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputSellSideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, unsigned short dcb, const char* dcCurve, SellSideValuationCashFlow& sSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputCurveData(unsigned int buyCurveDataSize, const char* buySideDcCurve, const double* buyYearFrac, const double* buyMarketData, unsigned int sellCurveDataSize, const char* sellSideDcCurve,  const double* sellYearFrac, const double* sellMarketData, std::vector<Curve>& curves);

void setDcRate(Girr& girr, std::vector<Curve>& curves);

void girrDeltaRiskFactor(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Girr>& girrs, double* resultGIRRDelta);

void bootstrapZeroCurveContinuous(std::vector<Curve>& curves);

double processNetPV(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves);

double processFxSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow);

void processGirrSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves, Girr& girr, double* resultGirrDelta, double* resultNetPvFxSensitivity);

void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves);

QuantLib::Real linterp(const std::vector<QuantLib::Real>& yearFractions, const std::vector<QuantLib::Real>& zeroRates, QuantLib::Real targetYearFraction);

QuantLib::Real calDomesticNetPV(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const char* buySideCurrency);

QuantLib::Real calFxSensitivity(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const char* buySideCurrency, const QuantLib::Real domesticNetPV);

QuantLib::Real calGirrSensitivity(const QuantLib::Real domesticNetPV, const QuantLib::Real oroginalNetPV);

QuantLib::DayCounter getDayCounterByDCB(unsigned short dcb, std::vector<QuantLib::DayCounter>& dayCounters);

std::vector<QuantLib::Real> getCurveYearFrac(const char* curveId, const std::vector<Curve>& curves);

std::vector<QuantLib::Real> getCurveZeroRate(const char* curveId, const std::vector<Curve>& curves);

/* FOR DEBUG */
std::string qDateToString(const QuantLib::Date& date);

void printAllInputData(long maturityDate, long revaluationDate, double exchangeRate, const char* buySideCurrency, double notionalForeign, unsigned short buySideDCB, const char* buySideDcCurve, unsigned int buyCurveDataSize, const double* buyCurveYearFrac, const double* buyMarketData, const char* sellSideCurrency, double notionalDomestic, unsigned short sellSideDCB, const char* sellSideDcCurve, unsigned int sellCurveDataSize, const double* sellCurveYearFrac, const double* sellMarketData, unsigned short calType, unsigned short logYn);

void printAllOutputData(const double* resultNetPvFxSensitivity, const double* resultGirrDelta);

void printAllData(const TradeInformation& tradeInformation, const BuySideValuationCashFlow& bSideCashFlow, const SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves, const std::vector<Girr>& girrs);

void printAllData(const TradeInformation& tradeInformation);

void printAllData(const BuySideValuationCashFlow& bSideCashFlow);

void printAllData(const SellSideValuationCashFlow& sSideCashFlow);

void printAllData(const std::vector<Curve>& curves);

void printAllData(const std::vector<Girr>& girrs);

#endif