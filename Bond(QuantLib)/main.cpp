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
    // QuantLib 라이브러리 사용 예제
    /* 부사장님 Algo Bond 입수 데이터 */
    /*
    const long evaluationDate = 45657;          // 2024-12-31
    const long settlementDays = 0;
    const long issueDate = 44175;
    const long maturityDate = 47827;
    const double notional = 6000000000.0;
    const double couponRate = 0.015;
    const int couponDayCounter = 5; //Actual/Actual(Bond)
    const long numberOfCpnSch = 12;
    const long paymentDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };
    const long realStartDates[] = { 45636, 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644 };
    const long realEndDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };
    const long numberOfGirrTenors = 10;
    const long girrDates[] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double girrRates[] = { 0.0337, 0.0317, 0.0285, 0.0272, 0.0269, 0.0271, 0.0278, 0.0272, 0.0254, 0.0222 };
    const long girrDayCounter = 1; // Actual/365
    const long girrInterpolator = 1; // Linear
    const long girrCompounding = 1; // Continuous
    const long girrFrequency = 1; // Annual
    const double spreadOverYield = 0.001389;
    const int spreadOverYieldCompounding = 1; // Continuous
    const int spreadOverYieldDayCounter = 1; // Actual/365
    const long numberOfCsrTenors = 5;
    const long csrDates[] = { 183, 365, 1095, 1825, 3650 };
    const double csrRates[] = { 0.0, 0.0, 0.0, 0.0005, 0.001 };
    */

    /* VBA 입수 데이터 */
    const long evaluationDate = 45657;          // 2024-12-31
    const long settlementDays = 0;              // 0
    const long issueDate = 45504;               // 2024-07-31
    const long maturityDate = 46599;            // 2027-07-31
    const double notional = 3000000.0;          
    const double couponRate = 0.055;
    const int couponDayCounter = 5; //Actual/Actual(Bond)
    const long numberOfCpnSch = 7;
    const long paymentDates[] = { 45688, 45869, 46052, 46234, 46416, 46598, 46783 };
    const long realStartDates[] = { 45504, 45688, 45869, 46052, 46234, 46416, 46598 };
    const long realEndDates[] = { 45688, 45869, 46052, 46234, 46416, 46598, 46783 };
    const long numberOfGirrTenors = 10;
    const long girrDates[] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
    const double girrRates[] = { 0.0434026344, 0.0552167219, 0.0539846356, 0.0403908251, 0.0527258415, 0.0524191116, 0.0526506237, 0.0405914923, 0.0403457538, 0.0377604176 };
    const long girrDayCounter = 1; // Actual/365
    const long girrInterpolator = 1; // Linear, 외삽 허용
    const long girrCompounding = 1; // Continuous
    const long girrFrequency = 1; // 쿠폰 지급 주기에 따라 Annual -> SemiAnnual로 변경(값 변동 없음)
    
    // CSR 스프레드 로직 VBA상 미구현으로 인한 CSR 커브 미적용 후 테스트
    const double spreadOverYield = 0.00;
    const int spreadOverYieldCompounding = 1; // Continuous
    const int spreadOverYieldDayCounter = 1; // Actual/365
    const long numberOfCsrTenors = 5; 
    const long csrDates[] = { 183, 365, 1095, 1825, 3650 };
    const double csrRates[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    
    //printSettlementDate(date, settlementDays);
    ZeroBondTest(
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
        girrDates,
        girrRates,
        girrDayCounter,
        girrInterpolator,
        girrCompounding,
        girrFrequency,
        spreadOverYield,
        spreadOverYieldCompounding,
        spreadOverYieldDayCounter,
        numberOfCsrTenors,
        csrDates,
        csrRates
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
