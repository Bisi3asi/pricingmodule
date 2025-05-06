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
    /* 부사장님 Algo 테스트 입수 데이터 */
    /*
    const double notional = 6000000000.0;       // 채권 원금 명목금액
    const long evaluationDate = 45657;          // 채권 평가 기준일 (serial number)
    const long settlementDays = 0;              // 결제일 offset 
    const long issueDate = 44175;               // 채권 발행일 (serial number)
	const long maturityDate = 47827;            // 채권 만기일 (serial number)
	const double couponRate = 0.015;            // 채권 쿠폰 이율 
    const int couponDayCounter = 5;             // DayCounter code (예: 5 = Actual/Actual(Bond))
    const long numberOfCpnSch = 12;             // 쿠폰 개수
    const long paymentDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };     // 지급일 배열
    const long realStartDates[] = { 45636, 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644 };   // 각 구간 시작일
	const long realEndDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };     // 각 구간 종료일
    const long numberOfGirrTenors = 10; // GIRR 만기 수
	const long girrDates[] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };    // GIRR 만기 (startDate로부터의 일수)
	const double girrRates[] = { 0.0337, 0.0317, 0.0285, 0.0272, 0.0269, 0.0271, 0.0278, 0.0272, 0.0254, 0.0222 }; // GIRR 금리
    const long girrDayCounter = 1;      // GIRR DayCounter (예: 1 = Actual/365)
    const long girrInterpolator = 1;    // 보간법 (예: 1 = Linear)
    const long girrCompounding = 1;     // 이자 계산 방식 (예: 1 = Continuous)
	const long girrFrequency = 1;       // Annual (예: 1 = Annual)
	const double spreadOverYield = 0.001389;    // 채권의 종목 Credit Spread
    const int spreadOverYieldCompounding = 1;   // Credit Spread 이자 계산 방식 (예: 1 = Continuous)
    const int spreadOverYieldDayCounter = 1;    // Credit Spread DayCounter (예: 1 = Actual/365)
	const long numberOfCsrTenors = 5;           // CSR 만기 수
	const long csrDates[] = { 183, 365, 1095, 1825, 3650 };     // CSR 만기 (startDate로부터의 일수)
	const double csrRates[] = { 0.0, 0.0, 0.0, 0.0005, 0.001 }; // CSR 금리 (금리 차이)
    */

	/* 부사장님 Algo 테스트 입수 데이터 */
    const double notional = 300000.0;           // 채권 원금 명목금액
    const long issueDateSerial = 45504;               // 채권 발행일 (serial number)
    const long revaluationDateSerial = 45657;         // 채권 평가 기준일 (serial number)
    const long maturityDateSerial = 46599;            // 채권 만기일 (serial number)
    const long settlementDays = 0;              // 결제일 offset 
    const long couponCnt = 6;                   // 쿠폰 개수
    const double couponRate = 0.055;            // 채권 쿠폰 이율 
    const int couponDCB = 1;                    // DayCounter code (예: 1 = Actual/Actual(Bond))
    const long realStartDatesSerial[] = { 45504, 45688, 45869, 46052, 46234, 46416 };   // 각 구간 시작일
    const long realEndDatesSerial[] = { 45688, 45869, 46052, 46234, 46416, 46598 };     // 각 구간 종료일
    const long paymentDatesSerial[] = { 45688, 45869, 46052, 46234, 46416, 46598 };     // 지급일 배열
    const long girrCnt = 10; // GIRR 만기 수
    const double girrRates[] = { 0.0434026344, 0.0552167219, 0.0539846356, 0.0403908251, 0.0527258415, 0.0524191116, 0.0526506237, 0.0405914923, 0.0403457538, 0.0377604176 }; // GIRR 금리
    const long girrDCB = 3;      // GIRR DayCounter (예: 1 = Actual/365)
    //const long girrInterpolator = 1;    // 보간법 (예: 1 = Linear)
    //const long girrCompounding = 1;     // 이자 계산 방식 (예: 1 = Continuous)
    //const long girrFrequency = 1;       // Annual (예: 1 = Annual)
    const double spreadOverYield = 0.001389;    // 채권의 종목 Credit Spread
    //const int spreadOverYieldCompounding = 1;   // Credit Spread 이자 계산 방식 (예: 1 = Continuous)
    const int spreadOverYieldDCB = 3;    // Credit Spread DayCounter (예: 1 = Actual/365)
    
    const long csrCnt = 5;           // CSR 만기 수
    const long csrTenorDays[] = { 183, 365, 1095, 1825, 3650 };     // CSR 만기 (startDate로부터의 일수)
    const double csrSpreads[] = { 0.0, 0.0, 0.0, 0.0005, 0.001 }; // CSR 금리 (금리 차이)

    const unsigned short logYn = 1;
    
    double resultGirrDelta[25];
	double resultCsrDelta[25];

    // OUTPUT


    pricing(
        notional,                    // 채권 원금 명목금액
        issueDateSerial,               // 발행일 (serial number)
        revaluationDateSerial,         // 평가일 (serial number)
        maturityDateSerial,            // 만기일 (serial number)
        settlementDays,                // 결제일 offset (보통 2일)
        
        couponCnt,                      // 쿠폰 개수
        couponRate,                  // 쿠폰 이율
        couponDCB,                      // DayCounter code (예: 5 = Actual/Actual(Bond))

        realStartDatesSerial,   // 각 구간 시작일
        realEndDatesSerial,     // 각 구간 종료일
        paymentDatesSerial,     // 지급일 배열

        girrCnt,                        // GIRR 만기 수
        girrRates,            // GIRR 금리
        girrDCB,                 // GIRR DayCounter (예: 1 = Actual/365)
        //girrInterpolator,               // 보간법 (예: 1 = Linear)
        //girrCompounding,                // 이자 계산 방식 (예: 1 = Continuous)
        //girrFrequency,                  // 이자 빈도 (예: 1 = Annual)

        spreadOverYield,             // 채권의 종목 Credit Spread
        //spreadOverYieldCompounding,     // Continuous
        spreadOverYieldDCB,      // Actual/365

        csrCnt,                         // CSR 만기 수
        csrSpreads,           // CSR 스프레드 (금리 차이)

        logYn,               // 로그 파일 생성 여부 (0: No, 1: Yes)

        resultGirrDelta,
        resultCsrDelta
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
