#include <iostream>
#include <iomanip>

#include "src/fxForward.h"

// �б⹮ ó��
#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__) || defined(__unix__)
    #include <unistd.h>
#endif

int main() {
    // �׽�Ʈ ������ ���� (TradeInformation)
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

    // �׽�Ʈ ������ ���� (buy Curve)
    const unsigned int buyCurveDataSize = 10;
    const double buyCurveYearFrac[buyCurveDataSize] = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
    const double buyMarketData[buyCurveDataSize] = { 2.64438703, 2.38058648, 2.10763173, 1.97593133, 1.98563969, 2.07148214, 2.25037149, 2.36128877, 2.34768987, 2.2255283 };

    // �׽�Ʈ ������ ���� (sell Curve)
    const unsigned int sellCurveDataSize = 10;
    const double sellCurveYearFrac[buyCurveDataSize] = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
    const double sellMarketData[buyCurveDataSize] = { 3.08, 2.58, 2.33, 2.19, 2.19, 2.23, 2.24, 2.12, 2.04, 2.04 };

    // �׽�Ʈ ������ ���� (cal Type, logYn)
	const unsigned short calType = 2; // 1: NetPV, 2: GIRR Sensitivity
	const unsigned short logYn = 1; // 0: No, 1: Yes

    // OUTPUT
    double resultNetPvFxSensitivity[2];
	double resultGirrDelta[25];

    pricing(
         maturityDate
        , revaluationDate
        , exchangeRate

        , buySideCurrency
        , notionalForeign
        , buySideDCB
        , buySideDcCurve
        
        , buyCurveDataSize 
        , buyCurveYearFrac
        , buyMarketData

        , sellSideCurrency
        , notionalDomestic
        , sellSideDCB
        , sellSideDcCurve
        
        , sellCurveDataSize
        , sellCurveYearFrac
        , sellMarketData

        , calType
        , logYn

		, resultNetPvFxSensitivity
        , resultGirrDelta
    );

    // ��� ���
    if (calType == 1) {
        // Index 0: Net PV
        std::cout << "Net PV: " << std::fixed << std::setprecision(2) << resultNetPvFxSensitivity[0] << std::endl;
        // Index 1: FX Sensitivity
        std::cout << "FX Sensitivity: " << std::fixed << std::setprecision(2) << resultNetPvFxSensitivity[1] << std::endl;
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

    // ȭ�� ���� ���� (������� ������ ȣȯ)
    #ifdef _WIN32
        system("pause");
    #else
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
    #endif

	return 0;
}

