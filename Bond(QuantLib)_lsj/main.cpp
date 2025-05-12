#include <iostream>
#include <iomanip>

#include "src/bond.h"

// 분기문 처리
#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__) || defined(__unix__)
    #include <unistd.h>
#endif

int main() {
    // 테스트 데이터 정의 (TradeInformation)
    const double notional = 3000000.0;
    const double exchangeRate = 1.0;
    const long issueDateSerial = 45504;
    const long maturityDateSerial = 46599;
    const long revaluationDateSerial = 45657;
    const double couponRate = 0.055;
    const int couponFrequencyMonth = 6;
    const int couponDcb = 1; // Act/Act
    const int businessDayConvention = 1; // Modified Following
    const int periodEndDateConvention = 1; // Adjusted
    const int accuralDcb = 3; // Act/365
    
    // 테스트 데이터 정의 (Curve)
    const char dcCurveId[20] = "IRUSD-RFR";
    const int dcCurveDataSize = 10;
    const double dcCurveYearFrac[dcCurveDataSize] = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
    const double dcCurveMarketData[dcCurveDataSize] = { 4.34026344, 5.52167219, 5.39846356, 4.03908251, 5.27258415, 5.24191116, 5.26506237, 4.05914923, 4.03457538, 3.77604176 };

    // TODO CRS, CSR 구현
    //const char crsCurveId[]

    // 테스트 데이터 정의 (logYn)
	const int logYn = 1; // 0: No, 1: Yes

    // OUTPUT
	double resultGirrDelta[25];

    double netPV = pricing(
        notional
        , exchangeRate
        , issueDateSerial
        , maturityDateSerial
        , revaluationDateSerial
        , couponRate
        , couponFrequencyMonth
        , couponDcb
        , businessDayConvention
        , periodEndDateConvention
        , accuralDcb

        , dcCurveId
        , dcCurveDataSize
        , dcCurveYearFrac
        , dcCurveMarketData

        , logYn

        , resultGirrDelta
    );

    // 결과 출력
    // Return Value : Net PV
    std::cout << "Net PV(소수점 고정): " << std::fixed << std::setprecision(10) << netPV << std::endl;
    std::cout << "Net PV(raw Double): " << netPV << std::endl;
    //    // index 0 : size
    //    std::cout << "GIRR Delta Size: " << static_cast<int>(resultGirrDelta[0]) << std::endl;
		
    //    for (int i = 1; i < static_cast<int>(resultGirrDelta[0]) + 1; ++i) {
	   //     // index 1 ~ size : GIRR Delta tenor
	   //     std::cout <<  i << ". GIRR Delta tenor: " << std::fixed << std::setprecision(2) << resultGirrDelta[i] << std::endl;
    //    }
    //    for (int i = static_cast<int>(resultGirrDelta[0]) + 1; i < static_cast<int>(resultGirrDelta[0]) * 2 + 1; ++i) {
    //        // size ~ end : GIRR Delta Sensitivity
    //        std::cout <<  i - static_cast<int>(resultGirrDelta[0]) << ". GIRR Delta Sensitivity: " << std::fixed << std::setprecision(2) << resultGirrDelta[i] << std::endl;
    //    }
    //}

    // 화면 종료 방지 (윈도우와 리눅스 호환)
    #ifdef _WIN32
        system("pause");
    #else
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
    #endif

	return 0;
}

