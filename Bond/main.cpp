#include <iostream>
#include <iomanip>

#include "src/bond.h"

// 분기문 처리
#ifdef _WIN32
#include <windows.h>`
#elif defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

int main() {
    // QuantLib 라이브러리 사용 예제
    /* 부사장님 Algo Bond 입수 데이터 */
    const int evaluationDate = 45657;          // 2024-12-31
    const int settlementDays = 0;
    const int issueDate = 44175;
    const int maturityDate = 47827;
    const double notional = 6000000000.0;
    const double couponRate = 0.015;
    const int couponDayCounter = 5; //Actual/Actual(Bond)
    const int numberOfCpnSch = 12;
    const int paymentDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };
    const int realStartDates[] = { 45636, 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644 };
    const int realEndDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };
    const int numberOfGirrTenors = 10;
    const int girrTenorDays[] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double girrRates[] = { 0.0337, 0.0317, 0.0285, 0.0272, 0.0269, 0.0271, 0.0278, 0.0272, 0.0254, 0.0222 };
    const int girrDayCounter = 1; // Actual/365
    const int girrInterpolator = 1; // Linear
    const int girrCompounding = 1; // Continuous
    const int girrFrequency = 1; // Annual
    const double spreadOverYield = 0.001389;
    const int spreadOverYieldCompounding = 1; // Continuous
    const int spreadOverYieldDayCounter = 1; // Actual/365
    const int numberOfCsrTenors = 5;
    const int csrTenorDays[] = { 183, 365, 1095, 1825, 3650 };
    const double csrRates[] = { 0.0, 0.0, 0.0, 0.0005, 0.001 };

    /* 테스트 데이터 */
    /*
    const int evaluationDate = 45657;          // 2024-12-31
    const int settlementDays = 0;              // 0
    const int issueDate = 45504;               // 2024-07-31
    const int maturityDate = 46599;            // 2027-07-31
    const double notional = 3000000.0;          
    const double couponRate = 0.055;
    const int couponDayCounter = 7; // 30U/360 BASIS (미구현, bond.cpp에서 변경)
    const int numberOfCpnSch = 6;
    const int paymentDates[] = { 45688, 45869, 46052, 46234, 46416, 46598 };
    const int realStartDates[] = { 45504, 45688, 45869, 46052, 46234, 46416 };
    const int realEndDates[] = { 45688, 45869, 46052, 46234, 46416, 46598 };
    const int numberOfGirrTenors = 10;
    const int girrDates[] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double girrRates[] = { 0.0434026344, 0.042590633, 0.041358547, 0.040390825, 0.040099753, 0.039793023, 0.040024535, 0.040591492, 0.040345754, 0.037760418 };
    const int girrDayCounter = 1; // Actual/365 (미구현, bond.cpp에서 변경)
    const int girrInterpolator = 1; // Linear, 외삽 허용
    const int girrCompounding = 1; // Continuous (미구현, bond.cpp에서 변경)
    const int girrFrequency = 1; // Annual (미구현, bond.cpp에서 변경)
    
    const double spreadOverYield = 0.0;
    const int spreadOverYieldCompounding = 1; // Continuous (미구현, bond.cpp에서 변경)
    const int spreadOverYieldDayCounter = 1; // Actual/365 (미구현, bond.cpp에서 변경)
    const int numberOfCsrTenors = 5; 
    const int csrDates[] = { 183, 365, 1095, 1825, 3650 };
    const double csrRates[] = { 0.012626089, 0.012626089, 0.012626089, 0.012626089, 0.012626089 };
    */
    
	const int calType = 3; // 계산 타입 (1: Price, 2. BASEL 2 Delta, 3. BASEL 3 GIRR / CSR) 
	const int logYn = 1; // 로그 파일 생성 (0: No, 1: Yes)

    double resultGirrDelta[50] = { 0 };
    double resultCsrDelta[50] = { 0 };

    double resultNetPV = pricing(
        evaluationDate,
        settlementDays,
        issueDate,
        maturityDate,
        notional,
        couponRate,
        couponDayCounter,
        numberOfCpnSch,
        paymentDates,
        realStartDates,
        realEndDates,
        numberOfGirrTenors,
        girrTenorDays,
        girrRates,
        girrDayCounter,
        girrInterpolator,
        girrCompounding,
        girrFrequency,
        spreadOverYield,
        spreadOverYieldCompounding,
        spreadOverYieldDayCounter,
        numberOfCsrTenors,
        csrTenorDays,
        csrRates,
        
        calType,
        logYn,

        resultGirrDelta,
        resultCsrDelta
    );

	// OUTPUT 1 결과 출력
	std::cout << "[Net PV]: " << std::setprecision(20) << resultNetPV << std::endl;
    std::cout << std::endl;

	// OUTPUT 2 결과 출력
    int girrSize = static_cast<int>(resultGirrDelta[0]);
    std::cout << "[GIRR Data Size]: " << girrSize << std::endl;     // index 0: size
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

    // OUTPUT 3 결과 출력
    // index 0 : csr delta size    
    int csrSize = static_cast<int>(resultCsrDelta[0]);
    std::cout << "[CSR Data Size]: " << csrSize << std::endl;     // index 0: size

    std::cout << "[CSR Delta Tenor]: ";
    for (int i = 0; i < csrSize; ++i) {
		std::cout << std::fixed << std::setprecision(2) << resultCsrDelta[i + 1]; // index 1 ~ size: tenor
        if (i != csrSize - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;

    std::cout << "[CSR Delta Sensitivity] " << std::endl;
    for (int i = 0; i < csrSize; ++i) {
        std::cout << "index " << (i + 1 + csrSize) << ": " << std::setprecision(20) << resultCsrDelta[i + 1 + csrSize] << std::endl; // index size + 1 ~ 2 * size: sensitivity
    }

    //for (int i = csrSize * 2 + 1; i < 50; ++i) {
    //    std::cout << i << ". Empty : " << resultCsrDelta[i] << std::endl; // Empty
    //}
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
