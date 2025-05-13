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
    const long maturityDateSerial = 45834;
    const long revaluationDateSerial = 45657;
    const double exchangeRate = 1532.578;

    const char buySideCurrency[4] = "EUR";
    const double notionalForeign = 30000;
    const unsigned short buySideDCB = 3;    // Act/365
    const char buySideDcCurveId[20] = "IREUR-CRS";

    const char sellSideCurrency[4] = "KRW";
    const double notionalDomestic = 45273000;
    const unsigned short sellSideDCB = 3;   // Act/365
    const char sellSideDcCurveId[20] = "IRKRW-CRS";

    // 테스트 데이터 정의 (buy Curve)
    const unsigned int buyCurveDataSize = 10;
    const double buyCurveYearFrac[buyCurveDataSize] = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
    const double buyMarketData[buyCurveDataSize] = { 2.64438703, 2.38058648, 2.10763173, 1.97593133, 1.98563969, 2.07148214, 2.25037149, 2.36128877, 2.34768987, 2.2255283 };

    // 테스트 데이터 정의 (sell Curve)
    const unsigned int sellCurveDataSize = 10;
    const double sellCurveYearFrac[buyCurveDataSize] = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
    const double sellMarketData[buyCurveDataSize] = { 2.9931427, 2.5760797, 2.3328592, 2.1926168, 2.1934282, 2.23125, 2.2380958, 2.115366, 2.0361275, 2.0361275 };

    // 테스트 데이터 정의 (cal Type, logYn)
	const unsigned short calType = 1; // 1: NetPV, 2: GIRR Sensitivity
	const unsigned short logYn = 1; // 0: No, 1: Yes

    // OUTPUT
    double resultNetPvFxSensitivity[2];
	double resultGirrDelta[25];

    pricing(
         maturityDateSerial
        , revaluationDateSerial
        , exchangeRate

        , buySideCurrency
        , notionalForeign
        , buySideDCB
        , buySideDcCurveId
        
        , buyCurveDataSize 
        , buyCurveYearFrac
        , buyMarketData

        , sellSideCurrency
        , notionalDomestic
        , sellSideDCB
        , sellSideDcCurveId
        
        , sellCurveDataSize
        , sellCurveYearFrac
        , sellMarketData

        , calType
        , logYn

		, resultNetPvFxSensitivity
        , resultGirrDelta
    );

    // 결과 출력
    if (calType == 1) {
        // Index 0: Net PV
        std::cout << "Net PV(소수점 고정): " << std::fixed << std::setprecision(10) << resultNetPvFxSensitivity[0] << std::endl;
        std::cout << "Net PV(raw Double): " << resultNetPvFxSensitivity[0] << std::endl;
        // Index 1: FX Sensitivity
        std::cout << "FX Sensitivity(소수점 고정): " << std::fixed << std::setprecision(10) << resultNetPvFxSensitivity[1] << std::endl;
        std::cout << "FX Sensitivity(raw Double): " << resultNetPvFxSensitivity[1] << std::endl;
    }
    else if (calType == 2) {
        // index 0 : size
        std::cout << "GIRR Delta Size: " << static_cast<int>(resultGirrDelta[0]) << std::endl;
		
        for (int i = 1; i < static_cast<int>(resultGirrDelta[0]) + 1; ++i) {
	        // index 1 ~ size : GIRR Delta tenor
	        std::cout <<  i << ". GIRR Delta tenor: " << std::fixed << std::setprecision(2) << resultGirrDelta[i] << std::endl;
        }
        for (int i = static_cast<int>(resultGirrDelta[0]) + 1; i < static_cast<int>(resultGirrDelta[0]) * 2 + 1; ++i) {
            // size ~ end : GIRR Delta Sensitivity
            std::cout <<  i - static_cast<int>(resultGirrDelta[0]) << ". GIRR Delta Sensitivity: " << std::fixed << std::setprecision(2) << resultGirrDelta[i] << std::endl;
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

