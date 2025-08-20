#include <iostream>
#include <iomanip>

#include "src/leg.h"

// 분기문 처리
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

int main() {
	/* Leg 테스트 */
    const int evaluationDate = 45107;   // 2023-06-30
    const int issueDate = 45107;        // 2023-06-30
    const int maturityDate = 45107;     // 2023-06-30
    const double notional = 3500;

    const int numberOfGirrTenors = 10;
    const int girrTenorDays[] = { 90, 180, 360, 720, 1080, 1800, 3600, 5400, 7200, 10800 };
    const double girrRates[] = { 0.04280017,0.04399676,0.04553207,0.04473308,0.04463471,0.04446805,0.04386471,0.04386471,0.04386471,0.04386471 };
    const int girrConvention[] = { 0, 0, 0, 0 }; // DayCounter, Interpolator, Compounding, Frequency

    const double girrRiskWeight = 0.017;

    const int calType = 2;
    const int logYn = 1;

    double resultGirrBasel2[5] = { 0 };
    double resultIndexGirrBasel2[5] = { 0 };
    double resultGirrDelta[23] = { 0 };
    double resultIndexGirrDelta[23] = { 0 };
    double resultGirrCvr[2] = { 0 };
    double resultIndexGirrCvr[2] = { 0 };
    double resultCashFlow[1000] = { 0 };

    double resultNetPV =

        /* Zero Coupon Leg */
        
        pricingZCL(
            evaluationDate, issueDate, maturityDate, notional,
            numberOfGirrTenors, girrTenorDays, girrRates, girrConvention,
            girrRiskWeight,
            calType, logYn,
            resultGirrBasel2, resultGirrDelta, resultGirrCvr, resultCashFlow
        );
        
        /* Fixed Rate Leg */
        /*
        pricingFDL(
            evaluationDate, issueDate, maturityDate, notional, couponRate, couponDayCounter,
            couponCalendar, couponFrequency, scheduleGenRule, paymentBDC, paymentLag, isNotionalExchange,
            numberOfCpnSch, paymentDates, realStartDates, realEndDates,
            numberOfGirrTenors, girrTenorDays, girrRates, girrConvention,
            girrRiskWeight,
            calType, logYn,
            resultGirrBasel2, resultGirrDelta, resultGirrCvr, resultCashFlow
        );
        */
        /* Floating Rate Leg */
        /*
        pricingFLL(
            evaluationDate, issueDate, maturityDate, notional, couponDayCounter, couponCalendar,
            couponFrequency, scheduleGenRule, paymentBDC, paymentLag, isNotionalExchange,
            fixingDays, gearing, spread, lastResetRate, nextResetRate,
            numberOfCpnSch, paymentDates, realStartDates, realEndDates,
            numberOfGirrTenors, girrTenorDays, girrRates, girrConvention,
            numberOfIndexGirrTenors, indexGirrTenorDays, indexGirrRates, indexGirrConvention, isSameCurve,
            indexTenor, indexFixingDays, indexCurrency, indexCalendar, indexBDC, indexEOM, indexDayCounter,
            girrRiskWeight,
            calType, logYn,
            resultGirrBasel2, resultIndexGirrBasel2, resultGirrDelta, resultIndexGirrDelta, resultGirrCvr, resultIndexGirrCvr, resultCashFlow
        );
        */

    // OUTPUT 1 결과 출력
    std::cout << "[Net PV]: " << std::setprecision(20) << resultNetPV << std::endl;
    std::cout << std::endl;

    // OUTPUT 2 결과 출력
    std::cout << "[GIRR Basel 2 Result] " << std::endl;
    std::cout << "Delta: " << std::setprecision(20) << resultGirrBasel2[0] << std::endl; // index 0: Delta
    std::cout << "Gamma: " << std::setprecision(20) << resultGirrBasel2[1] << std::endl; // index 1: Gamma
    std::cout << "Duration: " << std::setprecision(20) << resultGirrBasel2[2] << std::endl; // index 2: Duration
    std::cout << "Convexity: " << std::setprecision(20) << resultGirrBasel2[3] << std::endl; // index 3: Convexity
    std::cout << "PV01: " << std::setprecision(20) << resultGirrBasel2[4] << std::endl; // index 4: PV01
    std::cout << std::endl;

    // OUTPUT 3 결과 출력
    std::cout << "[Index GIRR Basel 2 Result] " << std::endl;
    std::cout << "Delta: " << std::setprecision(20) << resultIndexGirrBasel2[0] << std::endl; // index 0: Delta
    std::cout << "Gamma: " << std::setprecision(20) << resultIndexGirrBasel2[1] << std::endl; // index 1: Gamma
    std::cout << "Duration: " << std::setprecision(20) << resultIndexGirrBasel2[2] << std::endl; // index 2: Duration
    std::cout << "Convexity: " << std::setprecision(20) << resultIndexGirrBasel2[3] << std::endl; // index 3: Convexity
    std::cout << "PV01: " << std::setprecision(20) << resultIndexGirrBasel2[4] << std::endl; // index 4: PV01
    std::cout << std::endl;

    // OUTPUT 4 결과 출력
    int girrSize = static_cast<int>(resultGirrDelta[0]);
    std::cout << "[GIRR Delta Size]: " << girrSize << std::endl;     // index 0: size
    std::cout << "[GIRR Delta Tenor]: ";
    for (int i = 0; i < girrSize; ++i) {
        std::cout << std::fixed << std::setprecision(2) << resultGirrDelta[i + 1]; // index 1 ~ size: tenor
        if (i != girrSize - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
    std::cout << "[GIRR Delta Sensitivity] " << std::endl;
    for (int i = 0; i < girrSize; ++i) {
        std::cout << "index " << (i + 1 + girrSize) << ": " << std::setprecision(20) << resultGirrDelta[i + 1 + girrSize] << std::endl; // index size + 1 ~ 2 * size: sensitivity
    }

    //for (int i = girrSize * 2 + 1; i < 50; ++i) {
    //    std::cout << i << ". Empty : " << resultGirrDelta[i] << std::endl; // Empty
    //}
    std::cout << std::endl;

    // OUTPUT 5 결과 출력
    int indexGirrSize = static_cast<int>(resultIndexGirrDelta[0]);
    std::cout << "[Index GIRR Delta Size]: " << indexGirrSize << std::endl;     // index 0: size
    std::cout << "[Index GIRR Delta Tenor]: ";
    for (int i = 0; i < indexGirrSize; ++i) {
        std::cout << std::fixed << std::setprecision(2) << resultIndexGirrDelta[i + 1]; // index 1 ~ size: tenor
        if (i != indexGirrSize - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
    std::cout << "[Index GIRR Delta Sensitivity] " << std::endl;
    for (int i = 0; i < indexGirrSize; ++i) {
        std::cout << "index " << (i + 1 + indexGirrSize) << ": " << std::setprecision(20) << resultIndexGirrDelta[i + 1 + indexGirrSize] << std::endl; // index size + 1 ~ 2 * size: sensitivity
    }

    //for (int i = girrSize * 2 + 1; i < 50; ++i) {
    //    std::cout << i << ". Empty : " << resultGirrDelta[i] << std::endl; // Empty
    //}
    std::cout << std::endl;

    // OUTPUT 7 결과 출력
    std::cout << "[GIRR Curvature] " << std::endl;
    std::cout << "BumpUp Curvature: " << std::setprecision(20) << resultGirrCvr[0] << std::endl; // index 0: BumpUp Curvature
    std::cout << "BumpDown Curvature: " << std::setprecision(20) << resultGirrCvr[1] << std::endl; // index 1: BumpDown Curvature
    std::cout << std::endl;

    // OUTPUT 8 결과 출력
    std::cout << "[Index GIRR Curvature] " << std::endl;
    std::cout << "BumpUp Curvature: " << std::setprecision(20) << resultIndexGirrCvr[0] << std::endl; // index 0: BumpUp Curvature
    std::cout << "BumpDown Curvature: " << std::setprecision(20) << resultIndexGirrCvr[1] << std::endl; // index 1: BumpDown Curvature
    std::cout << std::endl;

    // 화면 종료 방지 (윈도우와 리눅스 호환)
    #ifdef _WIN32
    system("pause");
    #else
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    #endif

    return 0;
}
