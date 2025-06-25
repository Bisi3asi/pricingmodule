#include <iostream>
#include <vector>
#include <string>
#include "src/OtStock.h"

// 분기문 처리
#ifdef _WIN32
    #include <windows.h>
    #define EXPORT __declspec(dllimport) __stdcall
#elif defined(__linux__) || defined(__unix__)
    #include <unistd.h>
    #define EXPORT
#endif

//----------------------------------------------------------------
// stockPricing
// amt : 주식 시가
// price : 가격 ( 위험요인 O : 주식가격 , 위험요인 X : 주가 지수 ) - 시나리오 적용된 가격 또는 사후 검증의 금일 종가.
// basePricee : 기준가격 ( 위험요인 O : 주식 기준가격 , 위험요인 X : 주가지수 기준 가격 ) - 사후 검증 시 전일 종가.
// beta : 주식 베타 ( 위험요인 O : 1 , 위험요인 X : 주식 베타 ) 
// fx : 외화환율 ( 원화 포지션일 경우 0 )
// *p : 0:델타 , 1:감마, 2:베타, 3:세타, 4:로 
// *ResultPrice : 평가가격(이론가)
//----------------------------------------------------------------
int main() {
    // 입력값 설정
    double amt = 1200.0;          // 주식 시가
    double price = 1100.0;        // 현재 가격
    double basePrice = 1000.0;    // 기준 가격
    double beta = 1.2;            // 베타 값
    double fx = 0.0;              // 외환 환율 (원화 포지션일 경우 0)

    double p[5];             // 델타, 감마, 베가, 세타, 로 저장 배열
    double ResultPrice = 0.0;     // 평가 가격 저장 변수

    // 함수 호출
    long result = stockPricing(amt, price, basePrice, beta, fx, p, &ResultPrice);

    // 결과 출력
    std::cout << "Pricing Calculation Result:" << std::endl;
    std::cout << "  - Theoretical Price: " << ResultPrice << std::endl;
    std::cout << "  - Delta: " << p[0] << std::endl;
    std::cout << "  - Gamma: " << p[1] << std::endl;
    std::cout << "  - Vega: " << p[2] << std::endl;
    std::cout << "  - Theta: " << p[3] << std::endl;
    std::cout << "  - Rho: " << p[4] << std::endl;
    std::cout << "  - Return Code: " << result << std::endl;

    // 화면 종료 방지 (윈도우와 리눅스 호환)
#ifdef _WIN32
    system("pause");
#else
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
#endif

    return 0;
}
