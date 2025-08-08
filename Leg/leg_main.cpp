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
    const int evaluationDate = 45107;   // 2024-12-31
    const int issueDate = 41463;        // 2020-12-10
    const int maturityDate = 45117;     // 2030-12-10
    const double notional = 10000000000;
    const int couponDayCounter = 0;     //Actual/Actual(Bond)
    const int couponCalendar = 0;
    const int couponFrequency = 2;
    const int scheduleGenRule = 0;
    const int paymentBDC = 0;
    const int paymentLag = 0;
    const int isNotionalExchange = 1; // 0: 이자만 지급, others: 이자 + 원금 지급

    const int fixingDays = 0;
    const double gearing = 1.0;
    const double spread = 0.0;
    const double lastResetRate = 0.0353;
    const double nextResetRate = 0.0353;

    const int numberOfCpnSch = 0;
    const int paymentDates[] = { -1 };
    const int realStartDates[] = { -1 };
    const int realEndDates[] = { -1 };

    const int numberOfGirrTenors = 10;
    const int girrTenorDays[] = { 90, 180, 360, 720, 1080, 1800, 3600, 5400, 7200, 10800 };
    const double girrRates[] = { 0.03728534,0.03770668,0.03805505,0.03691913,0.03594992,0.03476204,0.03392737,0.03392737,0.03392737,0.03392737 };
    const int girrConvention[] = { 0, 0, 0, 0 }; // DayCounter, Interpolator, Compounding, Frequency

    const int numberOfIndexGirrTenors = 10;
    const int indexGirrTenorDays[] = { 90, 180, 360, 720, 1080, 1800, 3600, 5400, 7200, 10800 };
    const double indexGirrRates[] = { 0.03728534,0.03770668,0.03805505,0.03691913,0.03594992,0.03476204,0.03392737,0.03392737,0.03392737,0.03392737 };
    const int indexGirrConvention[] = { 0, 0, 0, 0 }; // DayCounter, Interpolator, Compounding, Frequency
    const int isSameCurve = 1; // Discounting Curve와 Index Curve의 일치 여부(0: False, others : true)

    const int indexTenor = 90;
    const int indexFixingDays = 1;
    const int indexCurrency = 0;
    const int indexCalendar = 0;
    const int indexBDC = 0;
    const int indexEOM = 0;
    const int indexDayCounter = 0;

    const double girrRiskWeight = 0.017;

    const int calType = 1;
    const int logYn = 1;

    double resultGirrBasel2[5] = { 0 };
    double resultIndexGirrBasel2[5] = { 0 };
    double resultGirrDelta[23] = { 0 };
    double resultIndexGirrDelta[23] = { 0 };
    double resultGirrCvr[2] = { 0 };
    double resultIndexGirrCvr[2] = { 0 };
    double resultCashFlow[1000] = { 0 };


    double resultNetPV =

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