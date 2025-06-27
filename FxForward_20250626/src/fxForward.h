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
#include <ql/cashflows/cashflows.hpp>
#include <ql/compounding.hpp>
#include <ql/instruments/bonds/zerocouponbond.hpp>
#include <ql/math/interpolations/linearinterpolation.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/date.hpp>

// dll export method
extern "C" {
    void EXPORT_API pricingFXF(
        // ===================================================================================================
        const int evaluationDateSerial              // INPUT 1.  평가일 (Revaluation Date)
        , const int settlementDays                  // INPUT 2.  (추가)결제일수
        , const double exchangeRate                 // INPUT 3.  현물환율 (Domestic / Foreign)  (Exchange Rate)
        , const int isBuySideDomestic			    // INPUT 4.  매입 통화 원화 여부 (0: No, 1: Yes)

        , const char* buySideCurrency               // INPUT 5.  매입 통화 (Buy Side Currency)
        , const double buySideNotional              // INPUT 6.  매입 명목금액 (Buy Side Notional)
        , const int buySidePayDate                  // INPUT 7.  (추가)매입 명목금액 지급일 (Buy Side Payment Date)
        , const int buySideDcb                      // INPUT 8.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const int buyCurveDataSize                // INPUT 9.  매입 커브 데이터 사이즈
        , const int* buyCurveTenorDays              // INPUT 10. 매입 커브 만기 기간 (Buy Curve Term)
        , const double* buyCurveRates               // INPUT 11. 매입 커브 마켓 데이터 (Buy Curve Market Data)

        , const char* sellSideCurrency              // INPUT 12. 매도 통화 (Sell Side Currency)
        , const double sellSideNotional             // INPUT 13. 매도 명목금액 (Sell Side Notional)
        , const int sellSidePayDate                 // INPUT 14. (추가)매도 명목금액 지급일 (Sell Side Payment Date)
        , const int sellSideDcb                     // INPUT 15. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const int sellCurveDataSize               // INPUT 16. 매도 커브 데이터 사이즈
        , const int* sellCurveTenorDays		        // INPUT 17. 매도 커브 만기 기간 (Sell Curve Term)
        , const double* sellCurveRates              // INPUT 18. 매도 커브 마켓 데이터 (Sell Curve Market Data)

        , const double buyGirrRiskWeight            // INPUT 19. 매입 GIRR 리스크요소 위험 가중치 (TODO)
        , const double sellGirrRiskWeight           // INPUT 20. 매도 GIRR 리스크요소 위험 가중치 (TODO)

        , const int calType                         // INPUT 21. 계산 타입 (1: Theo Price, 2. BASEL 2 Sensitivity, 3. BASEL 3 Sensitivity)
        , const int logYn                           // INPUT 22. 로그 파일 생성 여부 (0: No, 1: Yes)

        , double* resultNetPv                       // OUTPUT 1, 2. (수정) [index 0] Buy PV, [index 1] Sell PV
        , double* resultBuySideBasel2               // OUTPUT 3  (추가) Buy Side Basel2 Results(Delta, Gamma, Duration, Convexity, PV01)
        , double* resultSellSideBasel2              // OUTPUT 4  (추가) Sell Side Basel2 Results(Delta, Gamma, Duration, Convexity, PV01)
        , double* resultBuySideGirrDelta            // OUTPUT 5. Buy Side GIRR Delta [index 0: size, index 1 ~ end: 순서대로 buy Side tenor, buy Side Sensitivity, cs basis Sensitivity]
        , double* resultSellSideGirrDelta           // OUTPUT 6. Sell Side GIRR Delta [index 0: size, index 1 ~ end: 순서대로 sell Side tenor, sell Side Sensitivity, cs basis Sensitivity]
        , double* resultBuySideGirrCvr              // OUTPUT 7. (추가)Buy Side GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
        , double* resultSellSideGirrCvr             // OUTPUT 8. (추가)Sell Side GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
        // ===================================================================================================
	);
}

/* function */
void initResult(double* result, const int size);

void processResultArray(std::vector<QuantLib::Real> tenors, std::vector<QuantLib::Real> sensitivities, QuantLib::Size originalSize, double* resultArray);

double roundToDecimals(const double value, const int n);

#endif