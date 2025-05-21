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
        const long maturityDateSerial               // INPUT 1.  만기일 (Maturity Date) 
        , const long evaluationDateSerial           // INPUT 2.  평가일 (Revaluation Date)
        , const double exchangeRate                 // INPUT 3.  현물환율 (Domestic / Foreign)  (Exchange Rate)
        , const int isBuySideDomestic			    // INPUT 4.  매입 통화 원화 여부 (0: No, 1: Yes)     

        , const char* buySideCurrency               // INPUT 5.  매입 통화 (Buy Side Currency)
        , const double buySideNotional              // INPUT 6.  매입 명목금액 (Buy Side Notional)
        , const int buySideDcb                      // INPUT 7.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const int buyCurveDataSize                // INPUT 8.  매입 커브 데이터 사이즈
        , const int* buyCurveTenorDays              // INPUT 9.  매입 커브 만기 기간 (Buy Curve Term)  
        , const double* buyCurveRates               // INPUT 10. 매입 커브 마켓 데이터 (Buy Curve Market Data)

        , const char* sellSideCurrency              // INPUT 11. 매도 통화 (Sell Side Currency)
        , const double sellSideNotional             // INPUT 12. 매도 명목금액 (Sell Side Notional)
        , const int sellSideDcb                     // INPUT 13. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const int sellCurveDataSize               // INPUT 14. 매도 커브 데이터 사이즈
        , const int* sellCurveTenorDays		        // INPUT 15. 매도 커브 만기 기간 (Sell Curve Term)     
        , const double* sellCurveRates              // INPUT 16. 매도 커브 마켓 데이터 (Sell Curve Market Data) 

        , const int logYn                           // INPUT 17. 로그 파일 생성 여부 (0: No, 1: Yes)

        , double* resultNetPvFxSensitivity          // OUTPUT 1. [index 0] Net PV, [index 1] FX Sensitivity
        , double* resultGirrTenorSensitivity        // OUTPUT 2. GIRR Delta Tenor [index 0 ~ 10] Buy Side Tenor, [index 11 ~ 21] Buy Side Sensitivity, [index 22 ~ 32] Sell Side Tenor, [index 33 ~ 43] Sell Side Sensitivity 
        // ===================================================================================================
    );
}

/* struct */
struct TradeInformation {
    QuantLib::Date maturityDate;
    QuantLib::Date evaluationDate;
    QuantLib::Real exchangeRate;
	int isBuySideDomestic;
};

struct BuySideValuationCashFlow {
    char currency[4];
    QuantLib::Real principalAmount;
    QuantLib::Date cashFlowDate;
    QuantLib::Real yearFrac;
    int dcb;
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
    int dcb;
    QuantLib::Real dcRate;
    QuantLib::Real dcFactor;
    QuantLib::Real presentValue;
};

struct Curve {
	int sideFlag;
    QuantLib::Real rate;
    QuantLib::Real yearFrac;
    QuantLib::Real dcFactor;
    QuantLib::Real zeroRate;
};

struct Girr {
	int sideFlag;
    QuantLib::Real yearFrac;
    QuantLib::Real sensitivity;
};


/* function */
void initResult(double* result, int size);

void initDayCounters(std::vector<QuantLib::DayCounter>& dayCounters);

void inputTradeInformation(long maturityDateSerial, long evaluationDateSerial, double exchangeRate, int isBuySideDomestic, TradeInformation& tradeInformation);

void inputBuySideValuationCashFlow(const char* currency, double principalAmount, long evaluationDateSerial, long cashFlowDateSerial, int dcb, BuySideValuationCashFlow& bSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputSellSideValuationCashFlow(const char* currency, double principalAmount, long evaluationDateSerial, long cashFlowDateSerial, int dcb, SellSideValuationCashFlow& sSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputCurveData(const int evaluationDateSerial, const int buyCurveDataSize, const int* buyCurveTenorDays, const double* buyCurveRates, const int buySideDcb, const int sellCurveDataSize, const int* sellCurveTenorDays, const double* sellCurveRates, const int sellSideDcb, std::vector<QuantLib::DayCounter>& dayCounters, std::vector<Curve>& curves);

void setDcRate(Girr& girr, std::vector<Curve>& curves);

void setGirrDeltaCurve(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Girr>& girrs);

void bootstrapZeroCurveContinuous(std::vector<Curve>& curves);

double processNetPV(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves);

double processFxSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow);

void processGirrSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves, Girr& girr, double* resultNetPvFxSensitivity);

void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves);

QuantLib::Real linterp(const std::vector<QuantLib::Real>& yearFractions, const std::vector<QuantLib::Real>& zeroRates, QuantLib::Real targetYearFraction);

QuantLib::Real calDomesticNetPV(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const int isBuySideDomestic);

QuantLib::Real calFxSensitivity(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const double fxBumpRate, const int isBuySideDomestic, const QuantLib::Real domesticNetPV);

QuantLib::Real calGirrSensitivity(const QuantLib::Real domesticNetPV, const QuantLib::Real oroginalNetPV);

QuantLib::DayCounter getDayCounterByCode(const int code, std::vector<QuantLib::DayCounter>& dayCounters);

std::vector<QuantLib::Real> getCurveYearFrac(const int sideFlag, const std::vector<Curve>& curves);

std::vector<QuantLib::Real> getCurveZeroRate(const int sideFlag, const std::vector<Curve>& curves);

double roundToDecimals(double value, int n);

/* FOR DEBUG */
std::string qDateToString(const QuantLib::Date& date);

void printAllInputData(const long maturityDateSerial, const long evaluationDateSerial, const double exchangeRate, const int isBuySideDomestic, const char* buySideCurrency, const double buySideNotional, const int buySideDcb, const int buyCurveDataSize, const int* buyCurveTenorDays, const double* buyCurveRates, const char* sellSideCurrency, const double sellSideNotional, const int sellSideDcb, const int sellCurveDataSize, const int* sellCurveTenorDays, const double* sellCurveRates);

void printAllOutputData(const double* resultNetPvFxSensitivity, const double* resultGirrTenorSensitivity);

void printAllData(const TradeInformation& tradeInformation, const BuySideValuationCashFlow& bSideCashFlow, const SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves, const std::vector<Girr>& girrs);

void printAllData(const TradeInformation& tradeInformation);

void printAllData(const BuySideValuationCashFlow& bSideCashFlow);

void printAllData(const SellSideValuationCashFlow& sSideCashFlow);

void printAllData(const std::vector<Curve>& curves);

void printAllData(const std::vector<Girr>& girrs);

#endif