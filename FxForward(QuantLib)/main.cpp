#include <iostream>
#include <iomanip>

#include "src/fxForward.h"

// 분기문 처리
#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__) || defined(__unix__)
    #include <unistd.h>
#endif

int main() {
    // 테스트 데이터 정의 (TradeInformation)
    const long maturityDate = 46164;
    const long revaluationDate = 45657;
    const double exchangeRate = 1532.578;

    const char buySideCurrency[4] = "EUR";
    const double notionalForeign = 4576279.99;
    const unsigned short buySideDCB = 3;    // Act/365
    const char buySideDcCurve[20] = "IREUR-CRS";

    const char sellSideCurrency[4] = "KRW";
    const double notionalDomestic = 6820853799.5;
    const unsigned short sellSideDCB = 3;   // Act/365
    const char sellSideDcCurve[20] = "IRKRW-CRS";

    // 테스트 데이터 정의 (buy Curve)
    const unsigned int buyCurveDataSize = 10;
    const unsigned short buyCurveTerm[buyCurveDataSize] = { 3, 6, 1, 2, 3, 5, 10, 15, 20, 30 };
    const unsigned short buyCurveUnit[buyCurveDataSize] = { 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 };
    const double buyMarketData[buyCurveDataSize] = { 2.64438703, 2.38058648, 2.10763173, 1.97593133, 1.98563969, 2.07148214, 2.25037149, 2.36128877, 2.34768987, 2.2255283 };

    // 테스트 데이터 정의 (sell Curve)
    const unsigned int sellCurveDataSize = 10;
    const unsigned short sellCurveTerm[buyCurveDataSize] = { 3, 6, 1, 2, 3, 5, 10, 15, 20, 30 };
    const unsigned short sellCurveUnit[buyCurveDataSize] = { 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 };
    const double sellMarketData[buyCurveDataSize] = { 3.08, 2.58, 2.33, 2.19, 2.19, 2.23, 2.24, 2.12, 2.04, 2.04 };

    // OUTPUT
    double result[24];
    
    pricing(
         maturityDate
        , revaluationDate
        , exchangeRate

        , buySideCurrency
        , notionalForeign
        , buySideDCB
        , buySideDcCurve
        , buyCurveDataSize
        , buyCurveTerm
        , buyCurveUnit
        , buyMarketData

        , sellSideCurrency
        , notionalDomestic
        , sellSideDCB
        , sellSideDcCurve
        , sellCurveDataSize
        , sellCurveTerm
        , sellCurveUnit
        , sellMarketData
        
        , result
    );

    // 결과 출력
    for (int i = 0; i < 24; ++i) {
        if (i == 0) {
            // Index 0: Net PV
            std::cout << "Index " << i << " - Net PV: " << std::fixed << std::setprecision(2) << result[i] << std::endl;
        }
        else if (i == 1) {
            // Index 1: FX Sensitivity
            std::cout << "Index " << i << " - FX Sensitivity: " << std::fixed << std::setprecision(2) << result[i] << std::endl;
        }
        else {
            // Index 2-23: GIRR Delta
            std::cout << "Index " << i << " - GIRR Delta " << i-1 << ": " << std::fixed << std::setprecision(2) << result[i] << std::endl;
        }
    }

    // 화면 종료 방지 (윈도우와 리눅스 호환)
    #ifdef _WIN32
        system("pause");
    #else
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
    #endif

	return 0;
}

