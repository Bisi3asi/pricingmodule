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
    // 테스트 데이터 정의 (기본정보)
    const int evaluationDate = 45657; // 2024-12-31
	const int settlementDays = 0; // 결제일수 (기본값 0)
    const int isBuySideDomestic = 0; // 0: 매도 통화 원화

    // 테스트 데이터 정의 (Buy Side Valuation CashFlow)
    const char buySideCurrency[4] = "EUR";
    const double buySideNotional = 30000;
    const int buySidePayDate = 45834; // 2026-05-22
    const int buySideDCB = 3;    // Act/365

    // 테스트 데이터 정의 (Sell Side Valuation CashFlow)
    const char sellSideCurrency[4] = "KRW";
    const double sellSideNotional = 452730000;
    const int sellSidePayDate = 45834; // 2026-05-22
    const int sellSideDCB = 3;   // Act/365

    // 테스트 데이터 정의 (Buy Curve)
    const int buyCurveDataSize = 10;
    const int buyCurveTenorDays[buyCurveDataSize] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double buyCurveRates[buyCurveDataSize] = { 0.0264438703, 0.0238058648, 0.0210763173, 0.0197593133, 0.0198563969, 0.0207148214, 0.0225037149, 0.0236128877, 0.0234768987, 0.022255283 };

    // 테스트 데이터 정의 (Sell Curve)
    const int sellCurveDataSize = 10;
    const int sellCurveTenorDays[sellCurveDataSize] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double sellCurveRates[sellCurveDataSize] = { 0.029931427, 0.025760797, 0.023328592, 0.021926168, 0.021934282, 0.0223125, 0.022380958, 0.02115366, 0.020361275, 0.020361275 };

    // 테스트 데이터 정의 (TODO, Girr Risk Weight)
    const double buyGirrRiskWeight = 0.05;
    const double sellGirrRiskWeight = 0.05;

    // 테스트 데이터 정의 (측정 구분 / 로그파일 생성여부)
    const int calType = 3; // 1: Theo Price, 2. BASEL 2 Sensitivity, 3. BASEL 3 Sensitivity
    const int logYn = 1; // 0: No, 1: Yes

    // OUTPUT
    double resultNetPv[2] = { 0 };
    double resultBuySideBasel2[50] = { 0 };
    double resultSellSideBasel2[50] = { 0 };
    double resultBuySideGirrDelta[50] = { 0 };
    double resultSellSideGirrDelta[50] = { 0 };
    double resultBuySideGirrCvr[50] = { 0 };
    double resultSellSideGirrCvr[50] = { 0 };

    pricingFFW(
        evaluationDate

        , buySideCurrency
        , buySideNotional
        , buySidePayDate
        , buySideDCB

        , buyCurveDataSize
        , buyCurveTenorDays
        , buyCurveRates

        , sellSideCurrency
        , sellSideNotional
        , sellSidePayDate
        , sellSideDCB

        , sellCurveDataSize
        , sellCurveTenorDays
        , sellCurveRates

        , buyGirrRiskWeight
		, sellGirrRiskWeight

        , calType
        , logYn

        , resultNetPv
        , resultBuySideBasel2
        , resultSellSideBasel2
        , resultBuySideGirrDelta
        , resultSellSideGirrDelta
        , resultBuySideGirrCvr
        , resultSellSideGirrCvr
    );

    // 화면 종료 방지 (윈도우와 리눅스 호환)
    #ifdef _WIN32
        system("pause");
    #else
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
    #endif

	return 0;
}

