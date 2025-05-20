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
        long maturityDateSerial                     // INPUT 1.  만기일 (Maturity Date) 
        , long revaluationDateSerial                // INPUT 2.  평가일 (Revaluation Date)
        , double exchangeRate                       // INPUT 3.  현물환율 (Domestic / Foreign)  (Exchange Rate)
        , int isBuySideDomestic					    // INPUT 4.  매입 통화 원화 여부 (0: No, 1: Yes)     

        , const char* buySideCurrency               // INPUT 5.  매입 통화 (Buy Side Currency)
        , double notionalForeign                    // INPUT 6.  매입 외화기준 명목금액 (Notional Foreign)
        , int buySideDCB                            // INPUT 7.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , int buyCurveDataSize                      // INPUT 8.  매입 커브 데이터 사이즈
        , const double* buyCurveYearFrac            // INPUT 9.  매입 커브 만기 기간 (Buy Curve Term)  
        , const double* buyMarketData               // INPUT 10. 매입 커브 마켓 데이터 (Buy Curve Market Data)

        , const char* sellSideCurrency              // INPUT 11. 매도 통화 (sell Side Currency)
        , double notionalDomestic                   // INPUT 12. 매도 원화기준 명목금액 (Notional Domestic)
        , int sellSideDCB                           // INPUT 13. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , int sellCurveDataSize                     // INPUT 14. 매도 커브 데이터 사이즈
        , const double* sellCurveYearFrac		    // INPUT 15. 매도 커브 만기 기간 (Sell Curve Term)     
        , const double* sellMarketData              // INPUT 16. 매도 커브 마켓 데이터 (Sell Curve Market Data) 

        , int logYn                                 // INPUT 17. 로그 파일 생성 여부 (0: No, 1: Yes)

        , double* resultNetPvFxSensitivity          // OUTPUT 1. [index 0] Net PV, [index 1] FX Sensitivity
        , double* resultGirrTenor                   // OUTPUT 2. GIRR Delta Tenor [index 0 ~ 10] {0(Basis), 3m, 6m, 1y, 2y, 3y, 5y, 10y, 15y, 20y, 30y}
        , double* resultGirrSensitivity 			// OUTPUT 3. GIRR Delta Sensitivity [index 0 ~ 10] Tenor에 해당하는 민감도
        // ===================================================================================================
    );
}

/* struct */
struct TradeInformation {
    QuantLib::Date maturityDate;
    QuantLib::Date revaluationDate;
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
    QuantLib::Real marketData;
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

void inputTradeInformation(long maturityDateSerial, long revaluationDateSerial, double exchangeRate, int isBuySideDomestic, TradeInformation& tradeInformation);

void inputBuySideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, int dcb, BuySideValuationCashFlow& bSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputSellSideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, int dcb, SellSideValuationCashFlow& sSideCashFlow, std::vector<QuantLib::DayCounter>& dayCounters);

void inputCurveData(int buyCurveDataSize, const double* buyYearFrac, const double* buyMarketData, int sellCurveDataSize, const double* sellYearFrac, const double* sellMarketData, std::vector<Curve>& curves);

void setDcRate(Girr& girr, std::vector<Curve>& curves);

void setGirrDeltaCurve(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Girr>& girrs);

void bootstrapZeroCurveContinuous(std::vector<Curve>& curves);

double processNetPV(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves);

double processFxSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow);

void processGirrSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, std::vector<Curve>& curves, Girr& girr, double* resultNetPvFxSensitivity);

void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves);

QuantLib::Real linterp(const std::vector<QuantLib::Real>& yearFractions, const std::vector<QuantLib::Real>& zeroRates, QuantLib::Real targetYearFraction);

QuantLib::Real calDomesticNetPV(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const int isBuySideDomestic);

QuantLib::Real calFxSensitivity(const QuantLib::Real buySidePV, const QuantLib::Real sellSidePV, const QuantLib::Real exchangeRate, const char* buySideCurrency, const QuantLib::Real domesticNetPV);

QuantLib::Real calGirrSensitivity(const QuantLib::Real domesticNetPV, const QuantLib::Real oroginalNetPV);

QuantLib::DayCounter getDayCounterByDCB(int dcb, std::vector<QuantLib::DayCounter>& dayCounters);

std::vector<QuantLib::Real> getCurveYearFrac(const int sideFlag, const std::vector<Curve>& curves);

std::vector<QuantLib::Real> getCurveZeroRate(const int sideFlag, const std::vector<Curve>& curves);

double roundToDecimals(double value, int n);

/* FOR DEBUG */
std::string qDateToString(const QuantLib::Date& date);

void printAllInputData(long maturityDate, long revaluationDate, double exchangeRate, int isBuySideDomestic, const char* buySideCurrency, double notionalForeign, int buySideDCB, int buyCurveDataSize, const double* buyCurveYearFrac, const double* buyMarketData, const char* sellSideCurrency, double notionalDomestic, int sellSideDCB, int sellCurveDataSize, const double* sellCurveYearFrac, const double* sellMarketData,int logYn);

void printAllOutputData(const double* resultNetPvFxSensitivity, const double* resultGirrTenor, const double* resultGirrSensitivity);

void printAllData(const TradeInformation& tradeInformation, const BuySideValuationCashFlow& bSideCashFlow, const SellSideValuationCashFlow& sSideCashFlow, const std::vector<Curve>& curves, const std::vector<Girr>& girrs);

void printAllData(const TradeInformation& tradeInformation);

void printAllData(const BuySideValuationCashFlow& bSideCashFlow);

void printAllData(const SellSideValuationCashFlow& sSideCashFlow);

void printAllData(const std::vector<Curve>& curves);

void printAllData(const std::vector<Girr>& girrs);

#endif