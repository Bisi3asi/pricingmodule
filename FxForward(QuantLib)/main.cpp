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
    // 테스트 데이터 정의 (Trade Information)
    const long maturityDateSerial = 45834; // 2026-05-22
    const long revaluationDateSerial = 45657; // 2024-12-31
    const double exchangeRate = 1532.578;
	const int isBuySideDomestic = 0; // 0: 매도 통화 원화

	// 테스트 데이터 정의 (Buy Side Valuation CashFlow)
    const char buySideCurrency[4] = "EUR";
    const double buySideNotional = 30000;
    const int buySideDCB = 3;    // Act/365

	// 테스트 데이터 정의 (Sell Side Valuation CashFlow)
    const char sellSideCurrency[4] = "KRW";
    const double sellSideNotional = 45273000;
    const int sellSideDCB = 3;   // Act/365

    // 테스트 데이터 정의 (Buy Curve)
    const int buyCurveDataSize = 10;
    const int buyCurveTenorDays[buyCurveDataSize] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double buyCurveRates[buyCurveDataSize] = { 0.0264438703, 0.0238058648, 0.0210763173, 0.0197593133, 0.0198563969, 0.0207148214, 0.0225037149, 0.0236128877, 0.0234768987, 0.022255283 };

    // 테스트 데이터 정의 (Sell Curve)
    const int sellCurveDataSize = 10;
    const int sellCurveTenorDays[buyCurveDataSize] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double sellCurveRates[buyCurveDataSize] = { 0.029931427, 0.025760797, 0.023328592, 0.021926168, 0.021934282, 0.0223125, 0.022380958, 0.02115366, 0.020361275, 0.020361275 };

    // 테스트 데이터 정의 (측정 구분 / 로그파일 생성여부)
	const int calType = 3; // 1: Price, 2: BASEL 2 Delta, 3: BASEL 3 GIRR / CSR 
	const int logYn = 1; // 0: No, 1: Yes

    // OUTPUT
    double resultNetPvFxSensitivity[2];
    double resultGirrTenorSensitivity[44];

    pricing(
         maturityDateSerial
        , revaluationDateSerial
        , exchangeRate
        , isBuySideDomestic

		, buySideCurrency
        , buySideNotional
        , buySideDCB
        
        , buyCurveDataSize 
        , buyCurveTenorDays
        , buyCurveRates

        , sellSideCurrency
        , sellSideNotional
        , sellSideDCB
        
        , sellCurveDataSize
        , sellCurveTenorDays
        , sellCurveRates

        , calType
        , logYn

		, resultNetPvFxSensitivity
        , resultGirrTenorSensitivity
    );

    // OUTPUT 1 결과 출력
    // Index 0: Net PV 
    std::cout << "[Net PV]: " << std::fixed << std::setprecision(10) << resultNetPvFxSensitivity[0] << std::endl;
    // Index 1: FX Sensitivity
    std::cout << "[FX Sensitivity]: " << std::fixed << std::setprecision(10) << resultNetPvFxSensitivity[1] << std::endl;
    std::cout << std::endl;


    // OUTPUT 2 결과 출력
    // index 0 ~ 21: Buy Side
    std::cout << "[Buy Side GIRR Delta Tenor]: ";
    for (int i = 0; i < 11; ++i) {
        std::cout << std::fixed << std::setprecision(2) << resultGirrTenorSensitivity[i];
        if (i != 10) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
    std::cout << "[Buy Side GIRR Delta Sensitivity] " << std::endl;
    for (int i = 11; i < 22; ++i) {
		std::cout <<  "index " << i << ": " << std::fixed << std::setprecision(10) << resultGirrTenorSensitivity[i] << std::endl;
	}
	std::cout << std::endl;

    // index 22 ~ 43: Sell Side
    std::cout << "[Sell Side GIRR Delta Tenor]: ";
    for (int i = 22; i < 33; ++i) {
        std::cout << std::fixed << std::setprecision(2) << resultGirrTenorSensitivity[i];
        if (i != 32) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
    std::cout << "[Sell Side GIRR Delta Sensitivity] " << std::endl;
    for (int i = 33; i < 44; ++i) {
        std::cout << "index " << i << ": " << std::fixed << std::setprecision(10) << resultGirrTenorSensitivity[i] << std::endl;
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

